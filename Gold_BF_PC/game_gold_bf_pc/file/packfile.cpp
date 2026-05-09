#include "packfile.h"

#include "../util/unorgtypes.h"
#include "../util/powerpc.h"
#include "../engine/mem.h"
#include "../engine/string/engstring.h"
#include "../engine/engineinit.h"
#include "../Logger/Log.h"

#include "../thirdparty/zlib/zlib.h"

#define z_inflate       inflate
#define z_inflateReset  inflateReset
#define z_inflateEnd    inflateEnd
#define z_adler32       adler32
#define z_crc32         crc32
#define z_uncompress    uncompress

#define FILESEEK_END 0 
#define FILESEEK_ABS 1
static const char s_fopenModeRB[] = "rb";
static const char s_fopenModeWB[] = "wb";

packfileinfo s_packfiles[16];
fileInfo s_fileInfos[64];
_iobuf* s_fileHandles[64];
BYTE s_fileAccessAllowed = 1;
BYTE s_complainIfFileNotFound = 1;
tpak pak;
pakIpak ipak;

#define COMP_HEAD_MAX 8  // number of tpakHead slots in ipak

StreamInfo_s  s_Streams[300];
StreamInfo_s  s_ActiveStreams;
StreamInfo_s* s_CurStream = nullptr;
STask   s_tasks[512];

HANDLE s_hEvent = nullptr;

/*TASK VARS SEPERATE ONE DAY*/
unsigned int  s_freeTasks[512] = {};
unsigned int  dword_82CE2128 = 0;
unsigned int  s_threadJobQueue = (unsigned int)-1;
static const unsigned __int8 s_taskDefaultScheduleType[NUM_TASK_TYPES] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1  // .data:82A81284 (all 1 by default)
};


// Call this immediately after fileRead() fills a packfilehdr
static void swapPackfileHdr(packfilehdr* h)
{
    h->m_ftofs = bswap32(h->m_ftofs);
    h->m_ftsize = bswap32(h->m_ftsize);
    h->m_ofofs = bswap32(h->m_ofofs);
    h->m_ofsize = bswap32(h->m_ofsize);
    h->m_chunks = bswap16(h->m_chunks);
    h->m_seed = bswap16(h->m_seed);
    // m_magic is a char[4] — no swap needed
}

// Call this on each raw 5-DWORD entry before reShuffleft() unpacks it
// 'raw' points to the start of one 20-byte entry (5 DWORDs)
static void swapRawFtEntry(unsigned int* entry)
{
    entry[0] = bswap32(entry[0]);  // hash
    entry[1] = bswap32(entry[1]);  // filesize
    entry[2] = bswap32(entry[2]);  // packedsize
    entry[3] = bswap32(entry[3]);  // chunkpos
    entry[4] = bswap32(entry[4]);  // chunk | compchunkoffset bitfield
}

//custom helper
int findFreePackfileSlot()
{
    for (int i = 0; i < 16; i++)
    {
        if (!s_packfiles[i].m_used)
            return i;
    }
    return -1;
}

int packFileGetFileTableEntryNumberFromHash(packfileinfo* pf, unsigned int hash)
{
    DbgPrint("packFileGetFileTableEntryNumberFromHash");
    unsigned int* m_ht = pf->m_ht;
    packfileft* m_ft = pf->m_ft;
    int           lo = 0;
    int           hi = (int)pf->m_pf->m_ftsize - 1;

    if (hi < 0)
        return -1;

    // Binary search through the sorted hash table
    while (lo <= hi)
    {
        int          mid = (lo + hi) / 2;
        unsigned int midHash = m_ht[mid];

        if (hash == midHash)
        {
            if (m_ft[mid].m_packedsize == (unsigned int)-1)
                return -1;
            return mid;
        }
        else if (hash > midHash)
            lo = mid + 1;
        else
            hi = mid - 1;
    }

    return -1;
}

int packFileGetFileTableEntryNumberFromName(int idx, const char* filename)
{
    DbgPrint("packFileGetFileTableEntryNumberFromName");
    if (idx < 0)
        return -1;

    packfileinfo* pf = &s_packfiles[idx];

    char norm[256];
    int  out = 0;
    int  in = 0;

    do
    {
        char c = filename[in];
        char n = (c == '\\') ? '/' : (char)tolower(c);

        // Append unless this is a duplicate slash
        bool isDupSlash = (out > 0) && (norm[out - 1] == '/') && (n == '/');
        if (!isDupSlash)
            norm[out++] = n;

        if (!c)
            break;
        in++;
    } while (in < 256);

    // Jenkins-style hash over the normalised string using the pack's seed
    unsigned int seed = pf->m_pf->m_seed;
    unsigned int h = 0;

    for (const char* p = norm; *p; p++)
    {
        h = ((unsigned int)(norm[p - norm]) + seed) * 1025;
        seed = (h >> 6) ^ h;
    }

    // Final avalanche mix
    h = seed * 9;
    h = (h >> 11) ^ h;
    h = h * 32769;

    return packFileGetFileTableEntryNumberFromHash(pf, h);
}

//does the pack file exist in memory already
int packfilesFileExists(const char* filename, unsigned int* filenum)
{
    for (int i = 0; i < 16; i++)
    {
        if (!s_packfiles[i].m_used || !s_packfiles[i].m_initialised)
            continue;

        int entryNum = (int)packFileGetFileTableEntryNumberFromName(i, filename);
        *filenum = (unsigned int)entryNum;

        if (entryNum != -1)
            return i;
    }

    return -1;
}

unsigned int fileOpenHW(const char* filename, char openflags)
{
    DbgPrint("fileOpenHW");
    // Find a free slot in s_fileHandles
    // Checks 4 slots per iteration (unrolled loop)
    int    slot = 0;
    FILE** p = &s_fileHandles[1];

    while (s_fileHandles[slot] != nullptr)
    {
        if (!p[0]) { slot += 1; break; }
        else if (!p[1]) { slot += 2; break; }
        else if (!p[2]) { slot += 3; break; }

        p += 4;
        slot += 4;

        if (slot >= 64)
            break;
    }

    if (slot == 64 || slot == -1)
        return (unsigned int)-1;

    // Select fopen mode based on openflags bit 0: 1=read, 0=write
    const char* mode = (openflags & 1) ? s_fopenModeRB : s_fopenModeWB;

    // Build full path: s_hwPath + filename, max 255 chars total
    int filenameLen = (int)strlen(filename);
    int hwPathLen = (int)strlen(s_hwPath);

    if (hwPathLen + filenameLen > 255)
        return (unsigned int)-1;

    char fullPath[256];
    strcpy(fullPath, s_hwPath);
    strcat(fullPath, filename);

    // Convert forward slashes to backslashes (Xbox 360 filesystem requirement)
    for (char* p = fullPath; *p; p++)
    {
        if (*p == '/')
            *p = '\\';
    }
    DbgPrint("Mounting: %s", fullPath);
    FILE* f = fopen(fullPath, mode);
    s_fileHandles[slot] = f;

    if (!f)
        return (unsigned int)-1;

    return (unsigned int)slot;
}

unsigned int fileOpen(const char* filename, int openflags)
{
    DbgPrint("fileOpen %s", filename);
    // Find a free fileInfo slot — one where both m_fh and m_pack are -1
    int       slot = 0;
    fileInfo* fi = s_fileInfos;

    while (true)
    {
        bool free = (fi->m_fh == (unsigned int)-1) && (fi->m_pack == -1);
        if (free)
            break;

        fi++;
        slot++;

        if ((int)fi >= (int)(fileInfo*)(s_fileInfos + 64))
            return (unsigned int)-1;
    }

    fileInfo* entry = &s_fileInfos[slot];

    // If opening for read (openflags==1), probe packfiles first
    if (openflags == 1)
    {
        entry->m_pack = packfilesFileExists(filename,
            (unsigned int*)&entry->m_packfilenum);
        DbgPrint("fileOpen: pack=%d filenum=%d for %s",
            entry->m_pack, entry->m_packfilenum, filename);
        entry->m_ofs = 0;
    }

    // If not in any packfile, open as a hardware (filesystem) file
    if (entry->m_pack == -1)
        entry->m_fh = fileOpenHW(filename, openflags);

    // Fail if neither succeeded
    if (entry->m_fh == (unsigned int)-1 && entry->m_pack == -1)
        return (unsigned int)-1;
    return (unsigned int)slot;
}

int fileSeek(unsigned int handle, int offset, unsigned int seektype)
{
    DbgPrint("fileSeek");
    if (handle > 0x40)
        return -1;

    fileInfo* fi = &s_fileInfos[handle];

    int pack = fi->m_pack;

    if (pack == -1 && fi->m_fh != (unsigned int)-1)
    {
        // Hardware file seek via fileInfo slot
        FILE* f = s_fileHandles[fi->m_fh];
        if (!f) return -1;
        int whence = (seektype == FILESEEK_END) ? SEEK_END : SEEK_SET;
        fseek(f, offset, whence);
        return (int)ftell(f);
    }

    if (pack == -1 && fi->m_fh == (unsigned int)-1)
    {
        // Raw s_fileHandles index — called directly from pakStreamingReadEx
        FILE* f = s_fileHandles[handle];
        if (!f) return -1;
        int whence = (seektype == FILESEEK_END) ? SEEK_END : SEEK_SET;
        fseek(f, offset, whence);
        return (int)ftell(f);
    }

    if (pack < 0)
    {
        fi->m_ofs = 0;
        return 0;
    }

    // Pack file seek — clamp within [0, filesize]
    packfileft* ft = s_packfiles[pack].m_ft;
    int filesize = (int)ft[fi->m_packfilenum].m_filesize;
    int result = 0;

    if (seektype == FILESEEK_END)
    {
        fi->m_ofs = filesize;
        result = filesize;
    }
    else
    {
        fi->m_ofs = offset < 0 ? 0 : (offset > filesize ? filesize : offset);
        result = fi->m_ofs;
    }
    return result;
}

int fileClose(unsigned int handle)
{
    if (handle > 0x40)
        return 0;

    fileInfo* fi = &s_fileInfos[handle];
    if (!fi)
        return 0;

    // Close hardware file handle if open
    if (fi->m_fh != (unsigned int)-1)
    {
        fclose(s_fileHandles[fi->m_fh]);
        s_fileHandles[fi->m_fh] = nullptr;
    }

    // Decrement packfile reference count if backed by a packfile
    if (fi->m_pack != -1)
        s_packfiles[fi->m_pack].m_refCount--;

    // Reset slot to free
    fi->m_fh = (unsigned int)-1;
    fi->m_pack = -1;

    return 1;
}

int fileSize(const char* filename)
{
    DbgPrint("fileSize");
    int          result = -1;
    BYTE         savedComplain = s_complainIfFileNotFound;
    unsigned int filenum = 0;

    int packIdx = packfilesFileExists(filename, &filenum);

    if (packIdx == -1)
    {
        // Not in any packfile — open it directly as a hardware file
        s_complainIfFileNotFound = 0;
        unsigned int fh = fileOpen(filename, 1);
        s_complainIfFileNotFound = savedComplain;

        if (fh != (unsigned int)-1)
        {
            // Seek to end to get size
            result = fileSeek(fh, 0, FILESEEK_END);
            fileClose(fh);
        }
        return result;
    }
    return s_packfiles[packIdx].m_ft[filenum].m_filesize;
}

void NextActiveStream()
{
    StreamInfo_s* best = nullptr;

    for (StreamInfo_s* s = s_ActiveStreams.next;
        s != &s_ActiveStreams;
        s = s->next)
    {
        if (!best || s->gameframe < best->gameframe)
            best = s;
    }

    s_CurStream = best;
}

unsigned __int8 pakStreamingDone(tpak* ppak, const char* inDbgFileName)
{
    DbgPrint("pakStreamingDone");
    unsigned int handle = ppak->handle;

    // No handle — already done
    if (!handle)
        return 1;

    // Validate handle: check ipak.handles[(handle >> 8) & ~3] == handle
    // handle encodes the slot index in bits [8+]
    unsigned int handleSlotIdx = (handle >> 8) & 0xFFFFFC;
    if (*(unsigned int*)((char*)ipak.handles + handleSlotIdx) != handle)
    {
        // Stale handle — task already recycled
        ppak->handle = 0;
        return 1;
    }

    // Extract task slot index from handle (bits 10+)
    unsigned int handleIndex = handle >> 10;

    // Bounds check
    if (handleIndex >= 64)
    {
        g_assertsEnabled = 1;
        game_criticalAssert_fail(
            "handleIndex < _pakQueBufferCount",
            "[QA ASSERT!] THIS IS FATAL! THE GAME _WILL_ CRASH SOONER OR LATER!\n",
            "pakStreamingDone : handleIndex out of bounds",
            "source/pak/pakdecomp.c", 750,
            "pakStreamingDone");
    }

    // Look up task ID for this handle slot
    int taskID = ((unsigned int*)ipak.ptaskID)[handleIndex];
    if (taskID == -1)
        return 1;  // No task assigned — done

    // Check task status
    // status == 2 (complete) or status == 0 (idle) => done
    unsigned __int8 status = s_tasks[taskID].___u0.__s0.status;
    if (status == 2 || status == 0)
        return 1;

    // Task still running — check return code for errors
    if (ipak.returnCode[handleIndex] > 0)
    {
        // Decompression failed — format error message and assert
        int fileTableIdx = (int)((unsigned __int8*)ppak->fileStruct
            - (unsigned __int8*)ppak->packStruct->m_ft) >> 4;

        const char* msg = evafmt(
            "Pak decompression failed - return code %d whilst decompressing "
            "file '%s' : pak=%s entry=%d hash=0x%x size=%d compsize=%d",
            inDbgFileName,
            ppak->packStruct,
            fileTableIdx,
            ppak->packStruct->m_ht[fileTableIdx],
            ppak->fileStruct->m_filesize,
            ppak->fileStruct->m_packedsize);

        g_assertsEnabled = 1;
        game_criticalAssert_fail(
            "ipak.returnCode[handleIndex]<=0",
            "[QA ASSERT!] THIS IS FATAL! THE GAME _WILL_ CRASH SOONER OR LATER!\n",
            msg,
            "source/pak/pakdecomp.c", 768,
            "pakStreamingDone");
    }

    return 0;  // Still in progress
}

void pakTickTaskBody(tpakQue* ppakArgs)
{

    tpak* ppak = (tpak*)ppakArgs->item.ppak;
    unsigned __int8* headBuf = (unsigned __int8*)ppak->pheadBuffer;

    unsigned __int8* dest = (unsigned __int8*)ppak->pdestination;
    if (!dest)
        dest = (unsigned __int8*)ppak->ptail;

    DbgPrint("pakTickTaskBody: dest=%p ptail=%p length=%d",
        dest, ppak->ptail, ppak->length);

    DbgPrint("pheadBuffer=%p ptail=%p length=%d headLength=%d",
        ppak->pheadBuffer, ppak->ptail, ppak->length, ppak->headLength);
    DbgPrint("pheadBuffer[0..7]: %02X %02X %02X %02X %02X %02X %02X %02X",
        ((uint8_t*)ppak->pheadBuffer)[0], ((uint8_t*)ppak->pheadBuffer)[1],
        ((uint8_t*)ppak->pheadBuffer)[2], ((uint8_t*)ppak->pheadBuffer)[3],
        ((uint8_t*)ppak->pheadBuffer)[4], ((uint8_t*)ppak->pheadBuffer)[5],
        ((uint8_t*)ppak->pheadBuffer)[6], ((uint8_t*)ppak->pheadBuffer)[7]);

    // In PAK_TAIL mode headBuf overlaps dest — copy compressed data to safe temp
    unsigned __int8* tempBuf = nullptr;
    int headBufSize;
    if (ppak->mode == PAK_HEADTAIL)
    {
        headBufSize = 0xD000;
    }
    else
    {
        headBufSize = ppak->headLength;
        tempBuf = (unsigned __int8*)malloc(headBufSize);
        if (!tempBuf)
        {
            *ppakArgs->preturnCode = 7;
            return;
        }
        memcpy(tempBuf, headBuf, headBufSize);
        headBuf = tempBuf;
    }

    unsigned __int8* ot = ppak->packStruct->m_ot;
    int startOfs = ppak->offsetStart;
    int endOfs = ppak->offsetEnd;
    int startChunk = startOfs / 0x4000;
    int endChunk = endOfs / 0x4000;
    int startRem = startOfs % 0x4000;
    int endRem = endOfs % 0x4000;

    unsigned int bitIdx = 15 * (ppak->fileStruct->m_compchunkoffset + startChunk);

    if (ppak->mode == PAK_HEADTAIL)
    {
        int headLen = ppak->headLength;
        if (headLen > 0xD000)
            headLen = 0xD000;
        ppak->tailLength = ppak->headLength - headLen;
        ppak->ptailBuffer = (unsigned __int8*)ppak->pheadBuffer;
        ppak->tailOffset = ppak->headOffset;
        ppak->headLength = headLen;
        memmove(headBuf, ppak->pheadBuffer, headLen);
        ppak->tailLength -= headLen;
    }

    int headOfs = ppak->offset - ppak->headOffset;
    if (headOfs != 0)
    {
        memmove(headBuf, headBuf + headOfs, headBufSize - headOfs);
        int tailLen = ppak->tailLength;
        if (tailLen)
        {
            memmove(headBuf + headBufSize - headOfs,
                (unsigned __int8*)ppak->ptailBuffer + ppak->length - tailLen,
                headOfs);
            ppak->tailLength -= headOfs;
        }
    }

    int outPos = 0;
    for (int chunk = startChunk; chunk <= endChunk; chunk++)
    {
        unsigned int wordOfs = (bitIdx >> 3) & 0x1FFFFFFE;
        unsigned int bitOfs = bitIdx & 0xF;
        bitIdx += 15;

        int compSize = ((int)(otReadU32(ot + wordOfs) << bitOfs)) >> 17;
        DbgPrint("chunk=%d compSize=%d outPos=%d copyStart=%d copyEnd=%d",
            chunk, compSize, outPos,
            (chunk == startChunk) ? startRem : 0,
            (chunk == endChunk) ? endRem : 0x3FFF);
        unsigned __int8 chunkBuf[0x4000];
        uLongf chunkLen = 0x4000;

        if (compSize < 0)
        {
            int rawSize = -compSize;
            memmove(chunkBuf, headBuf, rawSize);
        }
        else
        {
            int zret = z_uncompress(chunkBuf, &chunkLen, headBuf, (unsigned int)compSize);
            DbgPrint("z_uncompress: ret=%d chunkLen=%lu compSize=%d headBuf[0..3]=%02X %02X %02X %02X",
                zret, chunkLen, compSize,
                headBuf[0], headBuf[1], headBuf[2], headBuf[3]);

            DbgPrint("chunkBuf[0..7]: %02X %02X %02X %02X %02X %02X %02X %02X",
                chunkBuf[0], chunkBuf[1], chunkBuf[2], chunkBuf[3],
                chunkBuf[4], chunkBuf[5], chunkBuf[6], chunkBuf[7]);
            if (zret != 0)
            {
                if (tempBuf) free(tempBuf);
                *ppakArgs->preturnCode = 7;
                return;
            }
        }

        int copyStart = (chunk == startChunk) ? startRem : 0;
        int copyEnd = (chunk == endChunk) ? endRem : 0x3FFF;
        int copyLen = copyEnd - copyStart + 1;

        memmove(dest + outPos, chunkBuf + copyStart, copyLen);

        DbgPrint("chunk=%d wrote %d bytes to dest+%d, dest[outPos]=%02X dest[outPos+1]=%02X",
            chunk, copyLen, outPos,
            dest[outPos], dest[outPos + 1]);

        outPos += copyLen;

        if (chunk != endChunk)
        {
            int advance = (compSize < 0) ? -compSize : compSize;
            memmove(headBuf, headBuf + advance, headBufSize - advance);
            int tailLen = ppak->tailLength;
            if (tailLen)
            {
                memmove(headBuf + headBufSize - advance,
                    (unsigned __int8*)ppak->ptailBuffer + ppak->length - tailLen,
                    advance);
                ppak->tailLength -= advance;
            }
        }
    }

    if (tempBuf)
        free(tempBuf);

    *ppakArgs->preturnCode = 0;
}

void homCallback(void* taskID)
{
    unsigned int* id = (unsigned int*)taskID;
    s_tasks[*id].___u0.__s0.status = 0;
    *id = (unsigned int)-1;
}

void pakWrapperFunction(STask* task)
{
    DbgPrint("pakWrapperFunction called");  // <- add this

    tpakQue* pakArgs = &task->args.pakArgs;

    pakTickTaskBody(pakArgs);

    *pakArgs->preturnCode = 0;

    void (*cb)(void*) = task->completedCallback;
    task->___u0.__s0.status = 2;

    if (cb)
        cb(task->callbackArg);
}

void nullWrapper(STask* task)
{
    DbgPrint("nullWrapper %s", task->name);
    return;
}

typedef void (*TaskWrapperFunc)(STask*);
static const TaskWrapperFunc s_taskDispatch[NUM_TASK_TYPES] = {

    nullWrapper, //skinningWrapperFunction,        // SKINNING_TASK  = 0
    nullWrapper, //testWrapperFunction,            // TEST_TASK       = 1
    nullWrapper, //particleWrapperFunction,        // PARTICLE_TASK   = 2
    nullWrapper, //m0vWrapperFunction,             // M0V_TASK        = 3
    pakWrapperFunction,             // PAK_TASK        = 4
    /*
    linetestWrapperFunction,        // LINETEST_TASK   = 5
    frdLinetestWrapperFunction,     // FRD_LINETEST    = 6
    navRouteFindWrapperFunction,    // NAVROUTE_TASK   = 7
    clothWrapperFunction,           // CLOTH_TASK      = 8
    pmeshWrapperFunction,           // PMESH_TASK      = 9
    abmWrapperFuntion,              // ABM_TASK        = 10
    homWrapperFunction,             // HOM_TASK        = 11
    dataTransferWrapperFunction,    // DATA_TRANSFER   = 12
    */
};  // .data:82A81294

/* TASK BASED FUNCTIONS */

int taskmanStartTask(unsigned int     type,
    UTaskArgs* args,
    char             scheduleType,
    const char* name,
    unsigned int* pTaskID,
    void           (*completedCallback)(void*),
    void* callbackArg,
    ETaskFlags       flags)
{
    unsigned int freeCount = dword_82CE2128;
    unsigned int taskIdx = (unsigned int)-1;

    while (true)
    {
        if (freeCount == 0)
        {
            // Rebuild free list by scanning for idle/complete tasks
            freeCount = 0;
            unsigned int* freeSlot = s_freeTasks;

            for (unsigned int i = 0; i < 512; i++)
            {
                if (s_tasks[i].___u0.__s0.status == 0)
                {
                    *freeSlot++ = i;
                    freeCount++;
                }
            }

            // Fill remainder of s_freeTasks with -1 sentinels
            if (freeCount < 512)
            {
                unsigned int* p = &s_freeTasks[freeCount];
                unsigned int  rem = 512 - freeCount;
                while (rem--)
                    *p++ = (unsigned int)-1;
            }
        }

        if (freeCount > 0)
        {
            // Pop a slot from the free list
            freeCount--;
            taskIdx = s_freeTasks[freeCount];
        }
        else
        {
            taskIdx = (unsigned int)-1;
        }

        if (taskIdx != (unsigned int)-1)
            break;

        // No free slots
        if (flags & k_taskflag_dontWaitForEmptySlot)
        {
            dword_82CE2128 = freeCount;
            return 0;
        }
        // Spin until a slot is free
    }

    dword_82CE2128 = freeCount;

    // If deferred scheduling, look up the per-type default scheduleType
    if (scheduleType == 2)
        scheduleType = s_taskDefaultScheduleType[type];

    // Fill in the task
    *pTaskID = taskIdx;
    STask* task = &s_tasks[taskIdx];
    memset(task, 0, sizeof(STask));

    task->___u0.__s0.id = taskIdx;
    task->___u0.__s0.status = 1;               // in-progress
    memcpy(&task->args, args, sizeof(UTaskArgs));
    task->___u0.__s0.type = (ETaskType)type;
    task->___u0.__s0.scheduleType = (unsigned __int8)scheduleType;
    strncpy(task->name, name, 16);
    task->completedCallback = completedCallback;
    task->callbackArg = callbackArg;
    task->name[15] = 0;

    if (scheduleType == 0)
    {
        // Immediate — call the wrapper directly on this thread
        s_taskDispatch[type](task);
    }
    else if (scheduleType == 1)
    {
        // Thread queue — hand off to worker thread
        tsQueueAdd(s_threadJobQueue, (char*)&taskIdx, 1);
    }
    // scheduleType == 2+ — deferred, handled elsewhere

    return 1;
}

/* MORE FILE FUNCTIONS */

void pakStreamingDecompress(tpak* ppak, unsigned __int8 immediate)
{
    DbgPrint("pakStreamingDecompress");
    // Find a free task ID slot in ipak.ptaskID (64 DWORDs, -1 = free)
    // The inner loop checks 4 slots at a time for efficiency
    int slot = 0;
    unsigned int* ids = (unsigned int*)ipak.ptaskID;

    // First pass: find a slot with taskID == -1
    while (slot < 64 && ids[slot] != (unsigned int)-1)
        slot++;

    // If all slots occupied, wait for one to become free
    // (checks task status — 0=idle or 2=complete means slot is reusable)
    while (slot == 64)
    {
        slot = 0;
        unsigned int* p = ids + 2;  // start at slot[2] scanning in groups of 4

        while (slot < 64)
        {
            // Check 4 consecutive slots for a done/idle task
            int done = -1;
            for (int sub = -2; sub <= 1; sub++)
            {
                int taskID = (int)p[sub];
                if (taskID == -1)
                {
                    done = slot + (sub + 2);
                    break;
                }
                unsigned __int8 status = s_tasks[taskID].___u0.__s0.status;
                if (status == 2 || status == 0)
                {
                    done = slot + (sub + 2);
                    break;
                }
            }

            if (done != -1)
            {
                slot = done;
                break;
            }

            slot += 4;
            p += 4;
        }
    }

    // Encode handle: slot in bits [10+], rolling counter in bits [0-9]
    unsigned int handle = ((unsigned int)slot << 10) | ipak.handleCounter;
    ipak.handleCounter++;

    // Roll counter back to 1 if the lower 10 bits overflow
    if ((ipak.handleCounter & ~0x3FFu) != 0)
        ipak.handleCounter = 1;

    // Register the handle in the slot
    ipak.handles[slot] = handle;
    ppak->handle = handle;

    // Store tpak pointer and return code pointer into task args
    // Task args buffer is an array of UTaskArgs (each 0x110 = 272 bytes)
    UTaskArgs* args = (UTaskArgs*)((char*)ipak.ptaskArgs + 272 * slot);
    args->pakArgs.item.ppak = ppak;
    args->pakArgs.preturnCode = &ipak.returnCode[slot];
    ipak.returnCode[slot] = -1;

    // Dispatch the decompression task
    if (immediate)
    {
        // Immediate: run synchronously on calling thread (scheduleType=0, name="pak")
        taskmanStartTask(PAK_TASK, args, 0, "pak",
            &ids[slot], homCallback,
            &ids[slot], ETaskFlags::k_taskflag_none);
    }
    else
    {
        // Deferred: schedule as background task (scheduleType=2, name="pak")
        taskmanStartTask(PAK_TASK, args, 2, "pak",
            &ids[slot], homCallback,
            &ids[slot], ETaskFlags::k_taskflag_none);
    }

    ppak->decomp = 1;
}

static const char s_pakDbgName[] = "";
void filePakUpdate()
{
    DbgPrint("filePakUpdate");
    StreamInfo_s* cur = s_CurStream;
    tpak* pak = &cur->pak;

    if (pak->decomp)
    {
        // Decompression in progress — poll for completion
        if (!pakStreamingDone(pak, s_pakDbgName))
            return;

        cur = s_CurStream;
    }
    else
    {
        epakMode mode = pak->mode;

        if (mode == PAK_RAW)
        {
            // Raw data — no decompression needed, fall through to unlink
        }
        else if (mode == PAK_HEADTAIL)
        {
            int tailLen = pak->tailLength;

            if (tailLen != 0)
            {
                if (tailLen > 0)
                {
                    // Still have tail data to read — set up next async read
                    cur->toread = tailLen;
                    cur->ovr.OffsetHigh = 0;
                    cur->ovr.hEvent = nullptr;
                    cur->buffer = (char*)pak->ptailBuffer;
                    cur->ovr.Offset = pak->tailOffset + cur->pakOffset;
                    cur->streamstate = 1;
                    pak->tailLength = -tailLen;
                }
                else
                {
                    // Tail read done — kick off decompression
                    cur->streamstate = 3;
                    pak->tailLength = -tailLen;
                    pakStreamingDecompress(pak, 0);
                }
                return;
            }

            // tailLength == 0 — fall through to decompress
            cur->streamstate = 3;
            pakStreamingDecompress(pak, 0);
            return;
        }
        else if ((unsigned int)mode >= (unsigned int)(PAK_TAIL | PAK_HEADTAIL))
        {
            // Unknown/invalid mode
            return;
        }
        else
        {
            // PAK_TAIL — kick off decompression
            cur->streamstate = 3;
            pakStreamingDecompress(pak, 0);
            return;
        }
    }

    // Unlink completed stream from active list and clear s_CurStream
    if (cur->prev) cur->prev->next = cur->next;
    if (cur->next) cur->next->prev = cur->prev;
    cur->prev = nullptr;
    cur->next = nullptr;
    s_CurStream = nullptr;
}

BYTE s_InitStreams = 1;
void fileTick()
{
    if (s_InitStreams)
    {
        for (int i = 0; i < 300; i++)
        {
            s_Streams[i].prev = nullptr;
            s_Streams[i].next = nullptr;
            s_Streams[i].streamstate = -1;
        }
        s_InitStreams = 0;
        s_ActiveStreams.next = &s_ActiveStreams;
        s_ActiveStreams.prev = &s_ActiveStreams;
        s_CurStream = nullptr;
        NextActiveStream();
        return;
    }

    StreamInfo_s* cur = s_CurStream;
    if (!cur)
    {
        NextActiveStream();
        return;
    }

    int state = cur->streamstate;

    if (state == 1)
    {
        unsigned int toRead = ((unsigned int)cur->toread + 2047) & ~2047u;
        if (toRead > 0x40000)
            toRead = 0x40000;

        cur->ovr.hEvent = s_hEvent;
        DWORD bytesRead = 0;
        BOOL  ok = ReadFile(cur->h, cur->buffer, toRead, &bytesRead, &cur->ovr);
        cur->gameframe = timerRenderFrameNum;

        if (ok)
        {
            cur->lastread = (int)bytesRead;

            if (toRead == bytesRead ||
                (unsigned int)(cur->ovr.Offset + bytesRead) >= (unsigned int)cur->upper)
            {
                // advance_stream
                cur->toread -= (int)bytesRead;
                cur->gameframe = 0;
                cur->buffer += cur->lastread;
                cur->ovr.Offset += cur->lastread;
                cur->streamstate = 1;
                if (cur->toread <= 0) cur->toread = 0;
            }

            // set_pending
            cur->toread -= (int)toRead;
            cur->gameframe = 0;
            cur->ovr.Offset += (int)bytesRead;
            cur->buffer += bytesRead;
            cur->streamstate = 1;
            if (cur->toread <= 0) cur->toread = 0;
            filePakUpdate();
            return;
        }

        DWORD err = GetLastError();
        if (err == ERROR_HANDLE_EOF)
        {
            if (bytesRead < toRead)
                return;
            if (cur->prev) cur->prev->next = cur->next;
            if (cur->next) cur->next->prev = cur->prev;
            cur->next = nullptr;
            cur->prev = nullptr;
            s_CurStream = nullptr;
        }
        else if (err == ERROR_IO_PENDING)
        {
            cur->lastread = (int)toRead;
            cur->streamstate = 2;
        }
        return;
    }
    else if (state == 2)
    {
        DWORD        bytesRead = 0;
        unsigned int lastRead = (unsigned int)cur->lastread;

        if (!GetOverlappedResult(cur->h, &cur->ovr, &bytesRead, FALSE))
        {
            GetLastError();
            return;
        }

        cur->lastread = (int)bytesRead;

        if (lastRead != bytesRead &&
            (unsigned int)(cur->ovr.Offset + bytesRead) < (unsigned int)cur->upper)
        {
            // set_pending
            cur->toread -= (int)lastRead;
            cur->gameframe = 0;
            cur->ovr.Offset += (int)bytesRead;
            cur->buffer += bytesRead;
            cur->streamstate = 1;
            if (cur->toread <= 0) cur->toread = 0;
            filePakUpdate();
            return;
        }

        // advance_stream
        cur->toread -= (int)lastRead;
        cur->gameframe = 0;
        cur->buffer += cur->lastread;
        cur->ovr.Offset += cur->lastread;
        cur->streamstate = 1;
        if (cur->toread <= 0) cur->toread = 0;

        // set_pending after advance
        cur->toread -= (int)lastRead;
        cur->gameframe = 0;
        cur->ovr.Offset += (int)bytesRead;
        cur->buffer += bytesRead;
        cur->streamstate = 1;
        if (cur->toread <= 0) cur->toread = 0;
        filePakUpdate();
        return;
    }
    else if (state == 3)
    {
        filePakUpdate();
        return;
    }

    NextActiveStream();
}

tpakHead* pakHeadAlloc()
{
    DbgPrint("pakHeadAlloc");
    static const double k_sleepMs = 0.003;  // ~3ms spin delay

    while (true)
    {
        // Search all 8 head slots for a free one
        for (int i = 0; i < COMP_HEAD_MAX; i++)
        {
            if (!ipak.head[i].used)
            {
                // Claim it
                ipak.head[i].used = 1;
                return &ipak.head[i];
            }
        }

        // All slots busy — pump the file system and spin-wait
        fileTick();

        if (!_isnan(k_sleepMs) && _finite(k_sleepMs))
            Sleep(3);

        DbgPrint("WARNING : Going into blocking state due to : increase COMP_HEAD_MAX "
            "or decrease amount of streaming files loaded simultaneously\n");
    }
}

unsigned int magic_0 = 0;
unsigned __int8 pakInitialise()
{
    DbgPrint("pakInitialise");
    // Already initialised — magic sentinel check
    if (magic_0 == 0xC0DEFACE)
        return 1;

    // Zero the entire ipak struct
    memset(&ipak, 0, sizeof(pakIpak));

    // Allocate task argument buffer (0x4400 bytes, 32-byte aligned)
    DbgPrint("ipak.ptaskArgs = memAllocAlignCore");
    ipak.handleCounter = 1;
    ipak.ptaskArgs = memAllocAlignCore(0x4400, 0, 32,
        "source/pak/pakdecomp.c", 154, "", 1);
    if (!ipak.ptaskArgs)
        return 0;
    DbgPrint("ipak.ptaskArgs = memAllocAlignCore done");
    // Allocate task ID array (256 bytes = 64 DWORDs, 32-byte aligned)

    DbgPrint("ipak.ptaskID = memAllocAlignCore");
    ipak.ptaskID = memAllocAlignCore(256, 0, 32,
        "source/pak/pakdecomp.c", 155, "", 1);
    DbgPrint("ipak.ptaskID = memAllocAlignCore done");
    if (!ipak.ptaskID)
    {
        memFreeFlags(ipak.ptaskArgs, 1);
        return 0;
    }

    // Initialise all task ID slots to -1 (free)
    unsigned int* ids = (unsigned int*)ipak.ptaskID;
    for (int i = 0; i < 64; i++)
        ids[i] = (unsigned int)-1;

    // Allocate decompression buffers for each head slot
    // Each is 0xD000 bytes, 2KB-aligned
    for (int i = 0; i < 8; i++)
    {
        DbgPrint("ipak.head[i].pbuffer = memAllocAlignCore");
        ipak.head[i].pbuffer = (unsigned __int8*)memAllocAlignCore(
            0xD000, 0, 0x800,
            "source/pak/pakdecomp.c", 801, "", 1);

        DbgPrint("ipak.head[i].pbuffer = memAllocAlignCore done");
    }

    // Mark as initialised
    magic_0 = 0xC0DEFACE;
    return 1;
}

//I can just add these to the function but im scares so i wont right now
int  one = 0;   // .data:82D10268 — PAK_TAIL reads in flight
int  two = 0;   // .data:82D1026C — PAK_HEADTAIL reads in flight

unsigned __int8 pakStreamingReadEx(tpak* ppak, unsigned __int8 immediate,
    unsigned __int8 nodecompress)
{
    DbgPrint("pakStreamingReadEx");
    DbgPrint("pakStreamingReadEx: file=%s length=%d offset=%d",
        ppak->fileStruct ? "valid_ft" : "null_ft",
        ppak->length, ppak->offsetStart);
    if (!pakInitialise())
        return 0;

    ppak->pdestination = nullptr;
    ppak->disablePartialRead = 0;

    packfileinfo* pf = ppak->packStruct;
    packfileft* ft = ppak->fileStruct;

    if (!pf || !ft->m_packedsize)
    {
        // Raw (uncompressed) file — head covers everything
        ppak->mode = PAK_RAW;
        ppak->headOffset = ppak->offsetStart;
        ppak->headLength = ppak->length;
        ppak->pheadBuffer = ppak->ptail;
        return 1;
    }

    // Clamp end offset to file size
    int offsetStart = ppak->offsetStart;
    int length = ppak->length;
    int offsetEnd = offsetStart + length - 1;
    if (offsetEnd >= (int)ft->m_filesize)
        offsetEnd = (int)ft->m_filesize - 1;
    ppak->offsetEnd = offsetEnd;

    // Walk the offset table (m_ot) to find the compressed byte offset
    // Each entry is 15 bits packed into m_ot; m_compchunkoffset gives the
    // starting bit index (= 15 * chunk index)
    unsigned int bitIdx = 15 * ft->m_compchunkoffset;
    unsigned __int8* ot = pf->m_ot;
    int           startChunk = offsetStart / 0x4000;
    int           endChunk = offsetEnd / 0x4000;

    // Accumulate compressed offset up to startChunk
    ppak->offset = 0;
    for (int i = 0; i < startChunk; i++)
    {
        int  wordOfs = (int)((bitIdx >> 3) & 0x1FFFFFFE);
        int  bitOfs = (int)(bitIdx & 0xF);
        bitIdx += 15;
        ppak->offset += abs(((int)(otReadU32(&ot[wordOfs]) << bitOfs)) >> 17);
    }

    // Accumulate compressed size over the range [startChunk, endChunk]
    int compSize = 0;
    for (int i = 0; i <= endChunk - startChunk; i++)
    {
        int  wordOfs = (int)((bitIdx >> 3) & 0x1FFFFFFE);
        int  bitOfs = (int)(bitIdx & 0xF);
        bitIdx += 15;
        compSize += abs(((int)(otReadU32(&ot[wordOfs]) << bitOfs)) >> 17);
    }

    DbgPrint("pakStreamingReadEx: startChunk=%d endChunk=%d compSize=%d headLen_before=%d length=%d",
        startChunk, endChunk, compSize,
        (ppak->offset & ~0x7FF) == 0 ? (compSize + 0x7FF) & ~0x7FF : 0,
        ppak->length);

    int  headOffset = ppak->offset & ~0x7FF;           // align down to 2KB
    int  headLen = (ppak->offset - headOffset + compSize + 0x7FF) & ~0x7FF;

    ppak->offset = headOffset;
    ppak->tailLength = 0;
    ppak->decomp = 0;

    if (headLen <= length)
    {
        // Fits in a single allocation — PAK_TAIL mode (data goes at end of dest)
        ppak->headOffset = headOffset;
        ppak->headLength = headLen;
        ppak->pheadBuffer = ppak->ptail + length - headLen;

        tpakHead* head = pakHeadAlloc();
        ppak->phead = head ? (unsigned __int8*)head : nullptr;
        if (!head)
            return 0;

        ppak->mode = PAK_TAIL;
        one++;
        return 1;
    }

    // Needs two allocations — PAK_HEADTAIL mode
    tpakHead* head = pakHeadAlloc();
    ppak->phead = (unsigned __int8*)head;
    if (!head)
        return 0;

    // Clamp head to max allocation size (0xD000 = 53248 bytes)
    int tailLen = headLen - 0xD000;
    if (tailLen < 0)
    {
        tailLen = 0;
    }
    else
    {
        headLen = 0xD000;
    }

    ppak->headOffset = headOffset;
    ppak->headLength = headLen;
    ppak->mode = PAK_HEADTAIL;
    ppak->pheadBuffer = head->pbuffer;
    two++;

    if (tailLen > 0)
    {
        // Tail spills into the destination buffer (at the far end)
        ppak->tailOffset = headOffset + headLen;
        ppak->tailLength = tailLen;
        ppak->ptailBuffer = ppak->ptail + length - tailLen;
        return 1;
    }

    // No tail needed — pheadBuffer alone covers it
    ppak->pheadBuffer = head->pbuffer;
    ppak->headLength = headLen;
    return 1;
}

int packfileRead(int idx, int fnum, int offset,
    unsigned __int8* buffer, int length)
{
    DbgPrint("packfileRead");
    if (idx < 0)
        return 0;

    packfileinfo* pf = &s_packfiles[idx];
    packfileft* ft = &pf->m_ft[fnum];

    // Fill out the pak streaming descriptor
    pak.ptail = buffer;
    pak.pdestination = nullptr;
    pak.offsetStart = offset;
    pak.length = length;
    pak.priority = 0;
    pak.packStruct = pf;
    pak.fileStruct = ft;
    pak.cacheMe = NO_CACHE;

    DbgPrint("packfileRead: buffer=%p length=%d", buffer, length);

    // Initiate read — returns 0 on failure
    if (!pakStreamingReadEx(&pak, 1, 0))
        return 0;

    // Seek to and read the head chunk
    int headPos = ft->m_chunkpos + pak.headOffset;
    unsigned int fh = pf->m_fh[ft->m_chunk];
    if (fileSeek(fh, headPos, FILESEEK_ABS) != headPos)
        return 0;
    fileRead(fh, pak.pheadBuffer, pak.headLength);

    DbgPrint("pak.mode=%d headLength=%d tailLength=%d tailOffset=%d",
        pak.mode, pak.headLength, pak.tailLength, pak.tailOffset);

    // If tail data exists, read it regardless of mode
    if (pak.tailLength > 0)
    {
        int tailPos = ft->m_chunkpos + pak.tailOffset;
        if (fileSeek(fh, tailPos, FILESEEK_ABS) != tailPos)
            return 0;
        fileRead(fh, pak.ptailBuffer, pak.tailLength);
    }

    if (pak.mode == PAK_RAW)
    {
        return length;
    }

    pakStreamingDecompress(&pak, 1);
    if (!pakStreamingDone(&pak, ""))
        return 0;

    return length;
}

unsigned int fileRead(unsigned int handle, unsigned __int8* buffer, unsigned int length)
{
    DbgPrint("fileRead");
    if (handle > 0x40)
        return (unsigned int)-1;

    fileInfo* fi = &s_fileInfos[handle];
    if (!fi)
        return (unsigned int)-1;

    if (fi->m_pack == -1 && fi->m_fh != (unsigned int)-1)
    {
        // Normal hardware file via fileInfo slot
        return (unsigned int)fread(buffer, 1, length, s_fileHandles[fi->m_fh]);
    }

    if (fi->m_pack == -1 && fi->m_fh == (unsigned int)-1)
    {
        // Raw s_fileHandles index — called directly from pakStreamingReadEx
        FILE* f = s_fileHandles[handle];
        if (!f) return (unsigned int)-1;
        return (unsigned int)fread(buffer, 1, length, f);
    }

    // Packfile read
    DbgPrint("fileRead: pack=%d packfilenum=%d ofs=%d length=%d",
        fi->m_pack, fi->m_packfilenum, fi->m_ofs, length);
    unsigned int bytesRead = (unsigned int)packfileRead(
        fi->m_pack, fi->m_packfilenum, fi->m_ofs, buffer, (int)length);
    fi->m_ofs += bytesRead;
    return bytesRead;
}

void reShuffleft(packfileinfo* pf, unsigned int* rawData)
{
    unsigned int ftSize = pf->m_pf->m_ftsize;

    for (unsigned int i = 0; i < ftSize; i++)
    {
        unsigned int* entry = rawData + i * 5;

        pf->m_ht[i] = bswap32(entry[0]);

        unsigned int* dst = (unsigned int*)&pf->m_ft[i];
        dst[0] = bswap32(entry[1]);  // m_filesize
        dst[1] = bswap32(entry[2]);  // m_packedsize
        dst[2] = bswap32(entry[3]);  // m_chunkpos
        dst[3] = entry[4];
    }
}

int packfileOpenAtIdx(const char* filename, unsigned __int8 loadmem, int idx)
{
    if (idx == -1)
        return -1;

    // Check the file exists on disk directly — bypass pack lookup
    unsigned int probeFh = fileOpenHW(filename, 1);
    if (probeFh == (unsigned int)-1)
    {
        DbgPrint("packfileOpenAtIdx: could not open %s", filename);
        return -1;
    }
    FILE* probeF = s_fileHandles[probeFh];
    fseek(probeF, 0, SEEK_END);
    int probeSize = (int)ftell(probeF);
    fclose(probeF);
    s_fileHandles[probeFh] = nullptr;

    if (probeSize <= 0)
    {
        DbgPrint("packfileOpenAtIdx: %s has size %d", filename, probeSize);
        return -1;
    }

    // Open the pak file for reading
    unsigned int fh = fileOpenHW(filename, 1);
    if (fh == (unsigned int)-1)
    {
        DbgPrint("packfileOpenAtIdx: second fileOpenHW failed for %s", filename);
        return -1;
    }
    FILE* f = s_fileHandles[fh];

    // Read header
    packfilehdr* hdr = (packfilehdr*)memAllocAlignCore(
        sizeof(packfilehdr), 0, 16,
        "source/file/packfile.c", 54, "", 1);
    if (!hdr)
    {
        fclose(f); s_fileHandles[fh] = nullptr;
        return -1;
    }

    if (fread(hdr, 1, sizeof(packfilehdr), f) != sizeof(packfilehdr))
    {
        DbgPrint("packfileOpenAtIdx: header read failed");
        memFreeFlags(hdr, 1);
        fclose(f); s_fileHandles[fh] = nullptr;
        return -1;
    }

    if (memcmp(hdr->m_magic, "PBCK", 4) != 0)
    {
        DbgPrint("packfileOpenAtIdx: bad magic in %s", filename);
        memFreeFlags(hdr, 1);
        fclose(f); s_fileHandles[fh] = nullptr;
        return -1;
    }

    // Byte-swap header
    swapPackfileHdr(hdr);
    DbgPrint("HDR: ftofs=%u ftsize=%u ofofs=%u ofsize=%u chunks=%u seed=%u",
        hdr->m_ftofs, hdr->m_ftsize, hdr->m_ofofs, hdr->m_ofsize,
        hdr->m_chunks, hdr->m_seed);

    // Seek to file table
    if (fseek(f, hdr->m_ftofs, SEEK_SET) != 0)
    {
        DbgPrint("packfileOpenAtIdx: seek to ftofs=%u failed", hdr->m_ftofs);
        memFreeFlags(hdr, 1);
        fclose(f); s_fileHandles[fh] = nullptr;
        return -1;
    }

    // Read raw file table — 5 DWORDs per entry (hash, filesize, packedsize, chunkpos, bits)
    unsigned int* ftRaw = nullptr;
    int ftRawBytes = hdr->m_ftsize * 20;
    if (ftRawBytes > 0)
    {
        ftRaw = (unsigned int*)memAllocAlignCore(ftRawBytes, 0, 16,
            "source/file/packfile.c", 54, "", 1);
        if (!ftRaw)
        {
            memFreeFlags(hdr, 1);
            fclose(f); s_fileHandles[fh] = nullptr;
            return -1;
        }

        if ((int)fread(ftRaw, 1, ftRawBytes, f) != ftRawBytes)
        {
            DbgPrint("packfileOpenAtIdx: ft raw read failed");
            memFreeFlags(ftRaw, 1);
            memFreeFlags(hdr, 1);
            fclose(f); s_fileHandles[fh] = nullptr;
            return -1;
        }
        DbgPrint("FT raw read OK: %d bytes", ftRawBytes);
    }

    // Allocate ft + ht in one block so m_ht = &m_ft[ftsize] works
    // ft: ftsize * 16 bytes (4 DWORDs per entry)
    // ht: ftsize * 4  bytes (1 DWORD per entry)
    packfileft* ft = nullptr;
    int ftBytes = hdr->m_ftsize * (int)sizeof(packfileft);
    int htBytes = hdr->m_ftsize * (int)sizeof(unsigned int);
    if (ftBytes > 0)
    {
        ft = (packfileft*)memAllocAlignCore(ftBytes + htBytes, 0, 16,
            "source/file/packfile.c", 54, "", 1);
        if (!ft)
        {
            if (ftRaw) memFreeFlags(ftRaw, 1);
            memFreeFlags(hdr, 1);
            fclose(f); s_fileHandles[fh] = nullptr;
            return -1;
        }
    }

    // Seek to and read offset table
    if (fseek(f, hdr->m_ofofs, SEEK_SET) != 0)
    {
        DbgPrint("packfileOpenAtIdx: seek to ofofs=%u failed", hdr->m_ofofs);
        if (ft)    memFreeFlags(ft, 1);
        if (ftRaw) memFreeFlags(ftRaw, 1);
        memFreeFlags(hdr, 1);
        fclose(f); s_fileHandles[fh] = nullptr;
        return -1;
    }

    unsigned __int8* ot = nullptr;
    if (hdr->m_ofsize > 0)
    {
        ot = (unsigned __int8*)memAllocAlignCore(hdr->m_ofsize, 0, 16,
            "source/file/packfile.c", 54, "", 1);
        if (!ot)
        {
            if (ft)    memFreeFlags(ft, 1);
            if (ftRaw) memFreeFlags(ftRaw, 1);
            memFreeFlags(hdr, 1);
            fclose(f); s_fileHandles[fh] = nullptr;
            return -1;
        }

        int otRead = (int)fread(ot, 1, hdr->m_ofsize, f);
        DbgPrint("OT read: wanted=%u got=%d", hdr->m_ofsize, otRead);
        if (otRead != (int)hdr->m_ofsize)
        {
            DbgPrint("packfileOpenAtIdx: ot read failed");
            memFreeFlags(ot, 1);
            if (ft)    memFreeFlags(ft, 1);
            if (ftRaw) memFreeFlags(ftRaw, 1);
            memFreeFlags(hdr, 1);
            fclose(f); s_fileHandles[fh] = nullptr;
            return -1;
        }
    }

    // Done with the pak file handle
    fclose(f);
    s_fileHandles[fh] = nullptr;

    // Fill slot
    packfileinfo* slot = &s_packfiles[idx];
    memset(slot, 0, sizeof(packfileinfo));

    strcpy(slot->m_name, filename);
    slot->m_inmem = 0;
    slot->m_used = 1;
    slot->m_initialised = 1;
    slot->m_refCount = 0;
    slot->m_pf = hdr;
    slot->m_ft = ft;
    slot->m_ht = (unsigned int*)&ft[hdr->m_ftsize];
    slot->m_ot = ot;
    slot->m_st = nullptr;
    slot->m_sto = nullptr;

    // Open chunk files
    for (unsigned int chunk = 0; chunk < hdr->m_chunks; chunk++)
    {
        char chunkName[352];
        sprintf_s(chunkName, sizeof(chunkName), "%s.%02d", filename, chunk);

        slot->m_fh[chunk] = fileOpenHW(chunkName, 1);
        slot->m_fsh[chunk] = fileOpenHW(chunkName, 1);
        slot->m_cfs[chunk] = nullptr;

        if (slot->m_fh[chunk] != (unsigned int)-1)
        {
            FILE* cf = s_fileHandles[slot->m_fh[chunk]];
            fseek(cf, 0, SEEK_END);
            slot->m_size[chunk] = (unsigned int)ftell(cf);
            fseek(cf, 0, SEEK_SET);
        }
        else
        {
            slot->m_size[chunk] = 0;
        }

        DbgPrint("  chunk[%u] fh=%u size=%u", chunk, slot->m_fh[chunk], slot->m_size[chunk]);
    }

    // Unpack raw ft into m_ft and m_ht with byte swapping
    if (ftRaw && ft)
    {
        for (unsigned int i = 0; i < hdr->m_ftsize; i++)
        {
            unsigned int* entry = ftRaw + i * 5;

            slot->m_ht[i] = bswap32(entry[0]);

            unsigned int* dst = (unsigned int*)&slot->m_ft[i];
            dst[0] = bswap32(entry[1]);  // m_filesize
            dst[1] = bswap32(entry[2]);  // m_packedsize
            dst[2] = bswap32(entry[3]);  // m_chunkpos
            dst[3] = bswap32(entry[4]);  // m_chunk | m_compchunkoffset
        }
        memFreeFlags(ftRaw, 1);
        ftRaw = nullptr;
    }

    DbgPrint("FT[0]: filesize=%u packedsize=%u chunk=%u chunkpos=%u",
        slot->m_ft[0].m_filesize, slot->m_ft[0].m_packedsize,
        slot->m_ft[0].m_chunk, slot->m_ft[0].m_chunkpos);
    DbgPrint("HT[0]: hash=0x%08X", slot->m_ht[0]);

    // Load string table from .str file
    char strPath[512];
    sprintf_s(strPath, sizeof(strPath), "%s%s.str", s_hwPath, filename);
    FILE* strF = fopen(strPath, "rb");
    if (strF)
    {
        fseek(strF, 0, SEEK_END);
        int strSize = (int)ftell(strF);
        fseek(strF, 0, SEEK_SET);

        int stoBytes = hdr->m_ftsize * 4;
        int stBytes = strSize - stoBytes;

        if (stBytes > 0)
        {
            int* sto = (int*)memAllocAlignCore(stoBytes, 0, 16,
                "source/file/packfile.c", 54, "", 1);
            char* st = (char*)memAllocAlignCore(stBytes, 0, 16,
                "source/file/packfile.c", 54, "", 1);

            if (sto && st)
            {
                fread(sto, 4, hdr->m_ftsize, strF);
                for (unsigned int i = 0; i < hdr->m_ftsize; i++)
                    sto[i] = (int)bswap32((unsigned int)sto[i]);

                fread(st, 1, stBytes, strF);

                slot->m_sto = sto;
                slot->m_st = st;
                DbgPrint("String table loaded: %u entries", hdr->m_ftsize);
            }
            else
            {
                if (sto) memFreeFlags(sto, 1);
                if (st)  memFreeFlags(st, 1);
            }
        }
        fclose(strF);
    }
    else
    {
        DbgPrint("No .str file found at %s", strPath);
    }

    DbgPrint("packfileOpenAtIdx SUCCESS: idx=%d ftsize=%u", idx, hdr->m_ftsize);
    return idx;
}

//This function is custom made, it uses the systems we have reversed to dump all of the files in the paks. A lot was helped with Claude but it has been tested, fixed reworked and exists mainly so I dont have to use quick bms and also to test the uh you know the uh the fucking oh yeah the actually uncompressed flipped data.
/*
void dumpAllPakFiles()
{
    for (int pakIdx = 0; pakIdx < 16; pakIdx++)
    {
        packfileinfo* slot = &s_packfiles[pakIdx];
        if (!slot->m_used || !slot->m_initialised)
            continue;

        DbgPrint("=== Dumping pak: %s (%u files) ===", slot->m_name, slot->m_pf->m_ftsize);

        for (unsigned int i = 0; i < slot->m_pf->m_ftsize; i++)
        {
            const char* name = nullptr;
            if (slot->m_st && slot->m_sto)
                name = slot->m_st + slot->m_sto[i];
            else
                continue;

            packfileft* ft = &slot->m_ft[i];
            unsigned int fileSize = ft->m_filesize;

            if (fileSize == 0)
                continue;

            if (ft->m_chunk >= slot->m_pf->m_chunks)
            {
                DbgPrint("  [%u] %s SKIP bad chunk=%u", i, name, ft->m_chunk);
                continue;
            }

            // Build output path and create directories
            char outPath[512];
            sprintf_s(outPath, sizeof(outPath), "dump\\%s", name);
            for (char* p = outPath; *p; p++)
                if (*p == '/') *p = '\\';

            char dirPath[512];
            strcpy_s(dirPath, outPath);
            for (char* p = dirPath + 1; *p; p++)
            {
                if (*p == '\\')
                {
                    char saved = *p; *p = '\0';
                    CreateDirectoryA(dirPath, nullptr);
                    *p = saved;
                }
            }

            // Allocate output buffer
            unsigned __int8* outBuf = (unsigned __int8*)malloc(fileSize);
            if (!outBuf) continue;

            FILE* cf = s_fileHandles[slot->m_fh[ft->m_chunk]];
            if (!cf) { free(outBuf); continue; }

            bool ok = false;

            if (!ft->m_packedsize)
            {
                // Uncompressed — read directly
                fseek(cf, (long)ft->m_chunkpos, SEEK_SET);
                ok = (fread(outBuf, 1, fileSize, cf) == fileSize);
            }
            else
            {
                // Compressed — read and decompress chunk by chunk manually
                unsigned __int8* ot = slot->m_ot;
                unsigned int cco = ft->m_compchunkoffset;
                int numChunks = (int)((fileSize + 0x3FFF) / 0x4000);
                unsigned int bitIdx = 15 * cco;

                // First pass: calculate compressed size of each chunk and total
                int* chunkSizes = (int*)malloc(numChunks * sizeof(int));
                if (!chunkSizes) { free(outBuf); continue; }

                bool boundsOk = true;
                int totalComp = 0;
                for (int c = 0; c < numChunks; c++)
                {
                    unsigned int wordOfs = (bitIdx >> 3) & 0x1FFFFFFE;
                    unsigned int bitOfs = bitIdx & 0xF;
                    bitIdx += 15;

                    if (wordOfs + 4 > slot->m_pf->m_ofsize)
                    {
                        DbgPrint("  [%u] %s OT out of bounds at chunk %d", i, name, c);
                        boundsOk = false;
                        break;
                    }

                    unsigned int val = bswap32(*(unsigned int*)(ot + wordOfs));
                    int cs = ((int)(val << bitOfs)) >> 17;
                    chunkSizes[c] = cs;
                    totalComp += abs(cs);
                }

                if (!boundsOk) { free(chunkSizes); free(outBuf); continue; }

                // Allocate compressed buffer
                unsigned __int8* compBuf = (unsigned __int8*)malloc(totalComp);
                if (!compBuf) { free(chunkSizes); free(outBuf); continue; }

                // Read all compressed data in one seek+read
                fseek(cf, (long)ft->m_chunkpos, SEEK_SET);
                fread(compBuf, 1, totalComp, cf);

                // Decompress each chunk
                ok = true;
                int compOfs = 0;
                int outOfs = 0;

                for (int c = 0; c < numChunks; c++)
                {
                    int cs = chunkSizes[c];

                    if (cs < 0)
                    {
                        // Stored uncompressed
                        int rawSize = -cs;
                        memcpy(outBuf + outOfs, compBuf + compOfs, rawSize);
                        outOfs += rawSize;
                        compOfs += rawSize;
                    }
                    else
                    {
                        // zlib decompress
                        uLongf destLen = 0x4000;
                        unsigned __int8 chunkOut[0x4000];
                        int zret = z_uncompress(chunkOut, &destLen,
                            compBuf + compOfs, (uLong)cs);
                        if (zret != Z_OK)
                        {
                            DbgPrint("  [%u] %s zlib error %d at chunk %d", i, name, zret, c);
                            ok = false;
                            break;
                        }
                        memcpy(outBuf + outOfs, chunkOut, destLen);
                        outOfs += (int)destLen;
                        compOfs += cs;
                    }
                }

                free(compBuf);
                free(chunkSizes);
            }

            if (ok)
            {
                FILE* outF = fopen(outPath, "wb");
                if (outF)
                {
                    fwrite(outBuf, 1, fileSize, outF);
                    fclose(outF);
                    DbgPrint("  [%u] dumped: %s (%u bytes)", i, name, fileSize);
                }
            }
            else
            {
                DbgPrint("  [%u] %s FAILED", i, name);
            }

            free(outBuf);
        }

        DbgPrint("=== Done dumping %s ===", slot->m_name);
    }
}

*/