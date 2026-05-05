#include "mem.h"
#include "../Logger/Log.h"
#include "string/engstring.h"
#include "util/unorgtypes.h"
#include "../util/mathf.h"

unsigned __int8 s_memInitDone = 0u;
unsigned int s_memMutex = (unsigned int)-1;
memgroupdef_s memgroups[64];

int s_memAllocsThisFrame = 0;
int g_heapSound = -1;
MutexSlot s_mutexSlots[16];

int g_heapDebug = -1;
dlinklistdef_s s_heapDebugOverFlow;
BYTE    g_assertsEnabled = 0;
mtx_u testMtx;
HANDLE* g_memMutexHandles;
memSettings_s memorySettingsG5 = { 45088768, 17825792, 0, 2306867, 1600, 15000, 24000, 76, 64, 44, 64 };
memSettings_s memorySettingsBF = { 32505856, 20971520, 1048576, 1258291, 1800, 10000, 10000, 76, 64, 44, 64 };

void* debugAlloc(int length, int inAlignment)
{
    DbgPrint("debugAlloc");
    // Minimum alignment is 16
    int align = (inAlignment >= 16) ? inAlignment : 16;

    // Allocate: enough room for the header + alignment padding + user data
    debugOverflow* hdr = (debugOverflow*)malloc(align + length + 16);
    if (!hdr)
        return nullptr;

    // Store raw malloc pointer in header
    hdr->alloc = hdr;

    // Align the user pointer past the header (header is 16 bytes, then align up)
    void* userPtr = (void*)(((unsigned int)hdr + align + 0xF) & ~(unsigned int)(align - 1));
    hdr->returnedAddress = userPtr;

    // Insert into s_heapDebugOverFlow list (prepend)
    dlinkdef_s* head = s_heapDebugOverFlow.head;
    hdr->link.prev = (dlinkdef_s*)&s_heapDebugOverFlow;
    hdr->link.next = head;
    s_heapDebugOverFlow.head = &hdr->link;
    head->prev = &hdr->link;

    return userPtr;
}

void groupLock(unsigned int mgh)
{
    //DbgPrint("groupLock");
    if (mgh != (unsigned int)-1 && mgh <= 0x3F)
    {
        memgroupdef_s* grp = &memgroups[mgh];
        if (grp->used && grp->groupMutex != (unsigned int)-1)
        {
            WaitForSingleObject(MUTEX_HANDLE(grp->groupMutex), INFINITE);
            return;
        }
    }
    WaitForSingleObject(MUTEX_HANDLE(s_memMutex), INFINITE);
}

void groupUnlock(unsigned int mgh)
{
    //DbgPrint("groupUnlock");
    if (mgh != (unsigned int)-1 && mgh <= 0x3F)
    {
        memgroupdef_s* grp = &memgroups[mgh];
        if (grp->used && grp->groupMutex != (unsigned int)-1)
        {
            ReleaseMutex(MUTEX_HANDLE(grp->groupMutex));
            return;
        }
    }
    ReleaseMutex(MUTEX_HANDLE(s_memMutex));
}


__int8 mutexIsInitialised = 0;
unsigned int mutexCreate()
{
    DbgPrint("mutexCreate");
    // One-time initialise all slots to zero
    if (!mutexIsInitialised)
    {
        memset(s_mutexSlots, 0, sizeof(s_mutexSlots));
        mutexIsInitialised = 1;
    }

    // Find a free slot
    int slot = 0;
    for (slot = 0; slot < 16; slot++)
    {
        if (!s_mutexSlots[slot].inUse)
            break;
    }

    // All slots used — return -1
    if (slot >= 16)
        return (unsigned int)-1;

    // Allocate the mutex
    HANDLE h = CreateMutexA(NULL, FALSE, NULL);
    s_mutexSlots[slot].handle = h;
    s_mutexSlots[slot].inUse = 1;
    return (unsigned int)slot;
}

void* memAllocAlignCore(unsigned int length, unsigned int mgh, int align,
    const char* file, int line, const char* comment,
    char inFlags)
{
    void* result = nullptr;

    s_memAllocsThisFrame++;

    if (!s_memInitDone)
        return malloc(length);

    groupLock(mgh);

    if (mgh != (unsigned int)-1 && mgh <= 0x3F)
    {
        memgroupdef_s* grp = &memgroups[mgh];

        if (grp->used)
        {
            MemGroupType type = grp->type;

            if (type == MEMGROUP_FIXEDSIZE)
            {
                poolState* pool = &grp->___u9.fixed.pool;
                DbgPrint("FIXEDSIZE mgh=%d freeCount=%d free=%p objectSize=%d",
                    mgh, pool->freeCount, pool->free, pool->objectSize);
                if (pool->freeCount > 0 && pool->free != nullptr)
                {
                    poolObject* obj = pool->free;
                    DbgPrint("  obj=%p obj->next=%p", obj, obj->next);
                    pool->free = obj->next;
                    pool->freeCount--;
                    result = obj;
                    goto done;
                }
                goto fallthrough;
            }
            else if (type == MEMGROUP_STACK)  // type == 2
            {
                // Stack allocator: bump-pointer from current stack top
                if (align < 16) align = 16;
                length = (length + 15) & ~15u;

                // Current block header sits at memblocks.tail + offset
                blockheaderdef_s* hdr = (blockheaderdef_s*)(
                    (char*)grp->___u9.stack.memblocks.tail
                    + grp->___u9.stack.memblocks.offset);

                if (!hdr->used
                    && hdr->size >= (int)((length + align + 27) & ~15u))
                {
                    hdr->flags = inFlags;

                    // Align the data pointer
                    char* data = (char*)(
                        ((unsigned int)((char*)hdr + align + 27)) & ~(unsigned int)(align - 1));
                    int usedBytes = (int)(data - (char*)hdr) + length;

                    *(int*)(data - 4) = (int)(data - (char*)hdr);
                    hdr->used = 1;

                    // Split remainder into a new free block if big enough
                    if (hdr->size - usedBytes >= 28)
                    {
                        blockheaderdef_s* next = (blockheaderdef_s*)((char*)hdr + usedBytes);
                        next->used = 0;
                        next->size = hdr->size - usedBytes;

                        // Re-link freelink list
                        dlinkdef_s* fwd = hdr->freelink.next;
                        next->freelink.prev = &hdr->freelink;
                        next->freelink.next = fwd;
                        hdr->freelink.next = &next->freelink;
                        fwd->prev = &next->freelink;

                        hdr->size = usedBytes;
                    }

                    grp->freeBytes -= usedBytes;
                    result = data;
                    if (result) goto done;
                }
            }
            else if (type == MEMGROUP_RANDOMSIZE)  // type == 0
            {
                // Random-size allocator: best-fit free block search
                if (align < 16) align = 16;
                length = (length + 15) & ~15u;

                int needed = (int)((length + align + 27) & ~15u);
                dlinklistdef_s* freeblocks = &grp->___u9.random.freeblocks;

                blockheaderdef_s* bestExact = nullptr;
                blockheaderdef_s* bestLarger = nullptr;
                int bestLargerSize = 0;
                int bestSmallerSize = 0;
                blockheaderdef_s* bestSmaller = nullptr;

                if ((dlinkdef_s*)freeblocks != freeblocks->head)
                {
                    int offset = freeblocks->offset;
                    dlinkdef_s* node = freeblocks->head;

                    do
                    {
                        blockheaderdef_s* blk = (blockheaderdef_s*)(
                            (char*)node + offset);
                        int blksize = blk->size;

                        if (blksize >= needed)
                        {
                            if (blksize - needed <= 16)
                            {
                                bestExact = blk;
                                goto found;
                            }
                            if (blksize > bestSmallerSize)
                            {
                                if (blksize <= bestLargerSize)
                                {
                                    bestSmaller = blk;
                                    bestSmallerSize = blksize;
                                }
                                else
                                {
                                    bestSmaller = bestLarger;
                                    bestSmallerSize = bestLargerSize;
                                    bestLarger = blk;
                                    bestLargerSize = blksize;
                                }
                            }
                        }
                        node = node->next;
                    } while ((dlinkdef_s*)freeblocks != node);

                    if (bestSmaller)
                    {
                        bestExact = bestSmaller;
                        goto found;
                    }
                }

                bestExact = bestLarger;
                if (!bestExact) goto fallthrough;

            found:
                {
                    bestExact->flags = inFlags;
                    char* data = (char*)(
                        ((unsigned int)((char*)bestExact + align + 27))
                        & ~(unsigned int)(align - 1));
                    int usedBytes = (int)(data - (char*)bestExact) + length;
                    *(int*)(data - 4) = (int)(data - (char*)bestExact);
                    bestExact->used = 1;

                    if (bestExact->size - usedBytes >= 28)
                    {
                        blockheaderdef_s* next = (blockheaderdef_s*)(
                            (char*)bestExact + usedBytes);
                        next->used = 0;
                        next->size = bestExact->size - usedBytes;

                        // Insert next into freeblocks list
                        dlinkdef_s* fbHead = freeblocks->head;
                        next->freelink.prev = (dlinkdef_s*)freeblocks;
                        next->freelink.next = fbHead;
                        freeblocks->head = &next->freelink;
                        fbHead->prev = &next->freelink;

                        // Re-link memblocks list
                        dlinkdef_s* mFwd = bestExact->link.next;
                        next->link.prev = &bestExact->link;
                        next->link.next = mFwd;
                        bestExact->link.next = &next->link;
                        mFwd->prev = &next->link;

                        bestExact->size = usedBytes;
                    }

                    // Unlink bestExact from freeblocks list
                    bestExact->freelink.next->prev = bestExact->freelink.prev;
                    bestExact->freelink.prev->next = bestExact->freelink.next;
                    bestExact->freelink.next = &bestExact->freelink;
                    bestExact->freelink.prev = &bestExact->freelink;

                    grp->freeBytes -= bestExact->size;
                    result = data;
                    if (result) goto done;
                }
            }
        }
    }

fallthrough:
    if (mgh == (unsigned int)g_heapDebug)
    {
        result = debugAlloc(length, align);
    }
    else
    {
        if (!mgh || mgh == (unsigned int)g_heapSound)
            goto done;

        if ((int)length <= 0)
        {
            groupUnlock(mgh);
            DbgPrint("memAllocAlignCore fallthrough nullptr");
            return nullptr;
        }

        // Fall back to heap 0 (global default heap)
        result = memAllocAlignCore(length, 0, align, file, line, comment, 0);
    }

done:
    //DbgPrint("memAllocAlignCore done");
    groupUnlock(mgh);

    return result;
}

void* memAllocAlignHighCore(int length, unsigned int mgh, int align,
    const char* file, int line, const char* comment,
    unsigned __int8 inFlags)
{
    void* result = nullptr;

    if (length <= 0)
        return nullptr;

    groupLock(mgh);

    if (mgh != (unsigned int)-1 && mgh <= 0x3F)
    {
        memgroupdef_s* grp = &memgroups[mgh];

        if (grp->used)
        {
            MemGroupType type = grp->type;

            if (type == MEMGROUP_FIXEDSIZE || type == MEMGROUP_STACK)
            {
                groupUnlock(mgh);
                return memAllocAlignCore(length, mgh, align, file, line, comment, 1);
            }
            else if (type == MEMGROUP_RANDOMSIZE)
            {
                if (align < 16) align = 16;

                int len16  = (length + 15) & ~15;
                int needed = (len16 + align + 27) & ~15;

                dlinklistdef_s* memblocks = &grp->___u9.random.memblocks;
                dlinkdef_s*     node      = memblocks->tail;
                int             offset    = memblocks->offset;

                if (node != (dlinkdef_s*)memblocks)
                {
                    bool found = false;
                    do
                    {
                        blockheaderdef_s* hdr = (blockheaderdef_s*)((char*)node + offset);

                        if (!hdr->used && hdr->size >= needed)
                        {
                            found     = true;
                            int mask  = ~(align - 1);
                            int remain = hdr->size - (mask & needed);

                            if (remain <= 32)
                            {
                                hdr->used  = 1;
                                hdr->flags = 1;
                                result     = (void*)((unsigned int)((char*)hdr + align + 27) & mask);
                                *(int*)((char*)result - 4) = (int)((char*)result - (char*)hdr);

                                hdr->freelink.next->prev = hdr->freelink.prev;
                                hdr->freelink.prev->next = hdr->freelink.next;
                                hdr->freelink.next       = &hdr->freelink;
                                hdr->freelink.prev       = &hdr->freelink;
                            }
                            else
                            {
                                char*             blockEnd  = (char*)hdr + hdr->size;
                                int               splitBase = (unsigned int)((char*)hdr + remain) & mask;
                                blockheaderdef_s* splitHdr  = (blockheaderdef_s*)splitBase;

                                splitHdr->used  = 1;
                                splitHdr->flags = 1;
                                result          = (void*)((splitBase + align + 27) & mask);
                                *(int*)((char*)result - 4) = (int)((char*)result - splitBase);

                                splitHdr->size = (int)(blockEnd - (char*)splitBase);
                                hdr->size      = splitBase - (int)hdr;

                                dlinkdef_s* splitLink = &splitHdr->link;
                                dlinkdef_s* fwd       = hdr->link.next;
                                splitLink->prev  = &hdr->link;
                                splitLink->next  = fwd;
                                hdr->link.next   = splitLink;
                                fwd->prev        = splitLink;
                            }
                            break;
                        }

                        node = node->prev;
                    } while (node != (dlinkdef_s*)memblocks);

                    if (found)
                        goto done;
                }
            }
        }
    }

    if (mgh == (unsigned int)g_heapDebug)
    {
        result = debugAlloc(length, align);
        goto done;
    }

    if (mgh != 0)
    {
        if (length <= 0)
        {
            groupUnlock(mgh);
            return nullptr;
        }
        result = memAllocAlignCore(length, 0, align, file, line, comment, 0);
    }

done:
    groupUnlock(mgh);
    return result;
}

int memAllocGroupRandomBlockSizeEx(const char* inName, int groupmemsize, unsigned __int8 allocLow)
{
    DbgPrint("memAllocGroupRandomBlockSizeEx");
    // Lock global memory mutex
    WaitForSingleObject(MUTEX_HANDLE(s_memMutex), INFINITE);

    // Find a free memgroups slot
    int slot = 0;
    while (memgroups[slot].used)
    {
        slot++;
        if ((int)&memgroups[slot] >= (int)&testMtx)
        {
            ReleaseMutex(MUTEX_HANDLE(s_memMutex));
            return -1;
        }
    }

    memgroupdef_s* grp = &memgroups[slot];

    // Copy name into slot (truncated to 16 chars)
    vafmtbuff(grp->name, 16, inName);

    // Allocate the raw memory block
    char* mem = (char*)memAllocAlignHighCore(groupmemsize, 0, 16,
        "source/mem/mem.c", 887,
        inName, 1);

    // Populate group descriptor
    grp->flags = 0;
    grp->type = MEMGROUP_RANDOMSIZE;
    grp->groupMutex = (unsigned int)-1;
    grp->groupstart = mem;
    grp->used = 1;
    grp->assertonfail = 1;
    grp->groupend = mem + groupmemsize;

    // Create a per-group mutex if name matches "physics" or "CLinetestMgr"
    if (strcmp(inName, "physics") == 0 || strcmp(inName, "CLinetestMgr") == 0)
        grp->groupMutex = mutexCreate();

    // Initialise random.freeblocks list (self-referential, offset=-8)
    grp->___u9.random.freeblocks.head = (dlinkdef_s*)&grp->___u9.random.freeblocks;
    grp->___u9.random.freeblocks.tail = (dlinkdef_s*)&grp->___u9.random.freeblocks;
    grp->___u9.random.freeblocks.offset = -8;

    // Initialise random.memblocks list (self-referential, offset=-16)
    grp->___u9.random.memblocks.head = (dlinkdef_s*)&grp->___u9.random.memblocks;
    grp->___u9.random.memblocks.tail = (dlinkdef_s*)&grp->___u9.random.memblocks;
    grp->___u9.random.memblocks.offset = -16;

    // Set up the initial blockheaderdef_s at the start of the allocated memory
    // It covers the entire region as one large free block
    blockheaderdef_s* hdr = (blockheaderdef_s*)mem;
    hdr->used = 0;
    hdr->size = groupmemsize;

    // freeBytes tracks available space
    grp->freeBytes = groupmemsize;

    // Link the first block's memblocks node into random.memblocks
    // (node sits at hdr + 0x10 = &hdr->link)
    dlinkdef_s* memNode = &hdr->link;
    memNode->next = grp->___u9.random.memblocks.head;
    memNode->prev = (dlinkdef_s*)&grp->___u9.random.memblocks;
    grp->___u9.random.memblocks.head = memNode;
    memNode->next->prev = memNode;

    // Link the first block's freeblocks node into random.freeblocks
    // (node sits at hdr + 0x08 = &hdr->freelink)
    dlinkdef_s* freeNode = &hdr->freelink;
    freeNode->next = grp->___u9.random.freeblocks.head;
    freeNode->prev = (dlinkdef_s*)&grp->___u9.random.freeblocks;
    grp->___u9.random.freeblocks.head = freeNode;
    freeNode->next->prev = freeNode;

    // Release global mutex
    ReleaseMutex(MUTEX_HANDLE(s_memMutex));

    return slot;
}

int memAllocGroupStack(const char* inName, int groupmemsize, int heap)
{
    DbgPrint("memAllocGroupStack");
    WaitForSingleObject(MUTEX_HANDLE(s_memMutex), INFINITE);

    // Find a free slot
    int slot = 0;
    while (memgroups[slot].used)
    {
        slot++;
        if ((int)&memgroups[slot] >= (int)&testMtx)
        {
            ReleaseMutex(MUTEX_HANDLE(s_memMutex));
            return -1;
        }
    }

    memgroupdef_s* grp = &memgroups[slot];

    vafmtbuff(grp->name, 16, inName);

    char* mem = (char*)memAllocAlignHighCore(groupmemsize, 0, 16,
        "source/mem/mem.c", 824,
        inName, 1);
    grp->groupstart = mem;

    if (!mem)
    {
        ReleaseMutex(MUTEX_HANDLE(s_memMutex));
        return -1;
    }

    grp->groupMutex = (unsigned int)-1;
    grp->groupend = mem + groupmemsize;
    grp->used = 1;
    grp->assertonfail = 1;
    grp->type = MEMGROUP_STACK;

    // Initialise stack.memblocks list (self-referential, offset=-16)
    grp->___u9.stack.memblocks.head = (dlinkdef_s*)&grp->___u9.stack.memblocks;
    grp->___u9.stack.memblocks.tail = (dlinkdef_s*)&grp->___u9.stack.memblocks;
    grp->___u9.stack.memblocks.offset = -16;

    // Set up the initial block header at groupstart
    // This acts as the stack's current top — size field tracks available space
    blockheaderdef_s* hdr = (blockheaderdef_s*)mem;
    hdr->used = 0;
    hdr->size = groupmemsize;

    hdr->freelink.next = &hdr->freelink;
    hdr->freelink.prev = &hdr->freelink;

    grp->freeBytes = groupmemsize;

    // Link hdr->link into stack.memblocks
    dlinkdef_s* head = grp->___u9.stack.memblocks.head;
    hdr->link.prev = (dlinkdef_s*)&grp->___u9.stack.memblocks;
    hdr->link.next = head;
    grp->___u9.stack.memblocks.head = &hdr->link;
    head->prev = &hdr->link;

    ReleaseMutex(MUTEX_HANDLE(s_memMutex));
    return slot;
}

void stackInit(stackheader_s* s, unsigned int numentries, unsigned int entrysize)
{
    if (!s)
        return;

    void* data = nullptr;
    if ((int)(numentries * entrysize) > 0)
        data = memAllocAlignCore(numentries * entrysize, 0, 0, __FILE__, 9, nullptr, 1);

    s->cur = 0;
    s->size = numentries;
    s->datasize = entrysize;
    s->data = data;
}

int g_heapTexStream = -1;
int g_scratchStack = -1;
ConditionSlot s_conditionSlots[32];

void memInitialise(unsigned __int8* buffer, unsigned int length)
{
    DbgPrint("memInitialise");
    s_memInitDone = 1;
    s_memMutex = mutexCreate();

    // Fill buffer with 0xCE pattern
    memset(buffer, 0xCE, length);

    // Zero the used flag on all memgroups slots
    for (memgroupdef_s* g = memgroups; (int)g < (int)&testMtx; g++)
        g->used = 0;

    // Set up memgroups[0] as the root heap covering the entire buffer
    vafmtbuff(memgroups[0].name, 16, "all/root");

    // Align groupstart to 16 bytes
    void* alignedBase = (void*)(((unsigned int)buffer + 15) & ~15u);

    memgroups[0].groupstart = alignedBase;
    memgroups[0].groupend = buffer + length;
    memgroups[0].used = 1;
    memgroups[0].type = MEMGROUP_RANDOMSIZE;
    memgroups[0].assertonfail = 0;
    memgroups[0].groupMutex = (unsigned int)-1;

    // Initialise memblocks list (self-referential, offset=-16)
    memgroups[0].___u9.random.memblocks.head = (dlinkdef_s*)&memgroups[0].___u9.random.memblocks;
    memgroups[0].___u9.random.memblocks.tail = (dlinkdef_s*)&memgroups[0].___u9.random.memblocks;
    memgroups[0].___u9.random.memblocks.offset = -16;

    // Initialise freeblocks list (self-referential, offset=-8)
    memgroups[0].___u9.random.freeblocks.head = (dlinkdef_s*)&memgroups[0].___u9.random.freeblocks;
    memgroups[0].___u9.random.freeblocks.tail = (dlinkdef_s*)&memgroups[0].___u9.random.freeblocks;
    memgroups[0].___u9.random.freeblocks.offset = -8;

    // Set up the initial blockheaderdef_s covering the entire aligned region
    blockheaderdef_s* hdr = (blockheaderdef_s*)alignedBase;
    hdr->used = 0;
    hdr->size = (int)((char*)memgroups[0].groupend - (char*)alignedBase);

    memgroups[0].freeBytes = hdr->size;

    g_heapTexStream = 0;

    // Link hdr->freelink into freeblocks list
    dlinkdef_s* freeHead = memgroups[0].___u9.random.freeblocks.head;
    hdr->freelink.prev = (dlinkdef_s*)&memgroups[0].___u9.random.freeblocks;
    hdr->freelink.next = freeHead;
    memgroups[0].___u9.random.freeblocks.head = &hdr->freelink;
    freeHead->prev = &hdr->freelink;

    // Link hdr->link into memblocks list
    dlinkdef_s* memHead = memgroups[0].___u9.random.memblocks.head;
    hdr->link.prev = (dlinkdef_s*)&memgroups[0].___u9.random.memblocks;
    hdr->link.next = memHead;
    memgroups[0].___u9.random.memblocks.head = &hdr->link;
    memHead->prev = &hdr->link;

    // Allocate the debug heap (32 bytes, in group 1)

    g_heapDebug = memAllocGroupRandomBlockSizeEx("debug", 32, 0);

    // Initialise the debug overflow tracking list
    s_heapDebugOverFlow.offset = 0;
    s_heapDebugOverFlow.head = (dlinkdef_s*)&s_heapDebugOverFlow;
    s_heapDebugOverFlow.tail = (dlinkdef_s*)&s_heapDebugOverFlow;

    // Allocate the scratch stack (0xC8000 = 819200 bytes)

    g_scratchStack = memAllocGroupStack("scratch", 0xC8000, 0);
}

memgroupdef_s* memGroupFindGroup(void* ptr)
{
    DbgPrint("memGroupFindGroup");
    // Search slots 1-63 first (slot 0 is the root heap, checked separately)
    for (int i = 1; i < 64; i++)
    {
        memgroupdef_s* grp = &memgroups[i];
        if (grp->used && ptr >= grp->groupstart && ptr < grp->groupend)
            return grp;
    }

    // Fallback: check slot 0 (the root heap covering the entire buffer)
    if (ptr >= memgroups[0].groupstart && ptr < memgroups[0].groupend)
        return &memgroups[0];

    return nullptr;
}

void memFreeFlags(void* mem, unsigned __int8 inFlags)
{
    DbgPrint("memFreeFlags");
    if (!mem || s_memMutex == (unsigned int)-1)
        return;

    memgroupdef_s* grp = memGroupFindGroup(mem);

    if (grp)
    {
        unsigned int slot = (unsigned int)(grp - memgroups);
        groupLock(slot);

        MemGroupType type = grp->type;

        if (type == MEMGROUP_FIXEDSIZE)
        {
            // Pool free — push block back onto memblocks free list
            *(void**)mem = grp->___u9.random.memblocks.tail;
            grp->___u9.random.memblocks.tail = (dlinkdef_s*)mem;
            grp->___u9.random.memblocks.offset++;
            groupUnlock(slot);
            return;
        }
        else if (type == MEMGROUP_STACK)
        {
            // Stack bump-pointer free — walk back the stack top pointer
            blockheaderdef_s* hdr = (blockheaderdef_s*)((char*)mem - *(int*)((char*)mem - 4));
            grp->freeBytes += hdr->size;
            hdr->used = 0;

            // Check if the block below (in address space) is also free — merge down
            dlinklistdef_s* memblocks = &grp->___u9.stack.memblocks;
            int             mbOffset = memblocks->offset;
            blockheaderdef_s* below = (blockheaderdef_s*)((char*)memblocks->tail + mbOffset);
            if (!below->used)
                hdr->size += below->size;

            groupUnlock(slot);
            return;
        }
        else if (type == MEMGROUP_RANDOMSIZE)
        {
            // Random-size free — mark block free and coalesce with neighbours
            blockheaderdef_s* hdr = (blockheaderdef_s*)((char*)mem - *(int*)((char*)mem - 4));

            hdr->used = 0;
            grp->freeBytes += hdr->size;

            int mbOffset = grp->___u9.random.memblocks.offset;
            int fbOffset = grp->___u9.random.freeblocks.offset;

            // Insert hdr->freelink into freeblocks list (prepend)
            dlinkdef_s* fbHead = grp->___u9.random.freeblocks.head;
            hdr->freelink.prev = (dlinkdef_s*)&grp->___u9.random.freeblocks;
            hdr->freelink.next = fbHead;
            grp->___u9.random.freeblocks.head = &hdr->freelink;
            fbHead->prev = &hdr->freelink;

            // Check block BEFORE hdr in memory (prev in memblocks) — coalesce if free
            blockheaderdef_s* prevHdr = (blockheaderdef_s*)
                ((char*)hdr->link.prev + mbOffset);

            dlinklistdef_s* sentinel = &grp->___u9.random.memblocks;
            if ((dlinkdef_s*)sentinel != hdr->link.prev && !prevHdr->used)
            {
                // Merge hdr into prevHdr
                prevHdr->size += hdr->size;

                // Unlink hdr from freeblocks
                hdr->freelink.next->prev = hdr->freelink.prev;
                hdr->freelink.prev->next = hdr->freelink.next;
                hdr->freelink.next = &hdr->freelink;
                hdr->freelink.prev = &hdr->freelink;

                // Unlink hdr from memblocks
                hdr->link.next->prev = hdr->link.prev;
                hdr->link.prev->next = hdr->link.next;
                hdr->link.next = &hdr->link;
                hdr->link.prev = &hdr->link;

                hdr = prevHdr;  // continue coalescing from prevHdr
            }

            // Check block AFTER hdr in memory (next in memblocks) — coalesce if free
            blockheaderdef_s* nextHdr = (blockheaderdef_s*)
                ((char*)hdr->link.next + mbOffset);

            if ((dlinkdef_s*)sentinel != hdr->link.next && !nextHdr->used)
            {
                // Merge nextHdr into hdr
                hdr->size += nextHdr->size;

                // Unlink nextHdr from freeblocks
                nextHdr->freelink.next->prev = nextHdr->freelink.prev;
                nextHdr->freelink.prev->next = nextHdr->freelink.next;
                nextHdr->freelink.next = &nextHdr->freelink;
                nextHdr->freelink.prev = &nextHdr->freelink;

                // Unlink nextHdr->link's freelink node (at hdr+8) from freeblocks
                dlinkdef_s* nl = (dlinkdef_s*)((char*)nextHdr + 8);
                nl->next->prev = nl->prev;
                nl->prev->next = nl->next;
                nl->next = nl;
                nl->prev = nl;
            }
        }

        groupUnlock(slot);
        return;
    }

    // Not found in any memgroup — check debug overflow list
    groupLock(g_heapDebug);

    dlinkdef_s* node = s_heapDebugOverFlow.head;

    if (node != (dlinkdef_s*)&s_heapDebugOverFlow)
    {
        do
        {
            debugOverflow* entry = (debugOverflow*)((char*)node
                + s_heapDebugOverFlow.offset);

            if (entry->returnedAddress == mem)
            {
                // Found — unlink from list
                node->next->prev = node->prev;
                node->prev->next = node->next;
                node->next = node;
                node->prev = node;

                // Free the original malloc'd block
                free(entry->alloc);
                break;
            }

            node = node->next;
        } while (node != (dlinkdef_s*)&s_heapDebugOverFlow);
    }

    groupUnlock(g_heapDebug);
}

void poolAddObjectsArray(poolState* ioPool, char* inObjectsArray, unsigned int inObjectCount)
{
    poolObject* free = ioPool->free;
    int objectSize = ioPool->objectSize & 0x0FFFFFFF;

    // Build a linked free list from the back of the array
    char* p = inObjectsArray + (inObjectCount - 1) * objectSize;
    if (inObjectCount)
    {
        unsigned int remaining = inObjectCount;
        do
        {
            poolObject* obj = (poolObject*)p;
            --remaining;
            p -= objectSize;
            obj->next = free;
            free = obj;
        } while (remaining);
    }

    ioPool->free = free;
    ioPool->freeCount += inObjectCount;
    ioPool->totalCount += inObjectCount;

    // Mark as non-contiguous if this array doesn't start where objects started
    if (inObjectsArray != (char*)ioPool->objects)
        ioPool->objectSize |= 0x40000000;
}

int memAllocGroupFixedBlockSize(
    const char* inName,
    int blockallocsize,
    unsigned int numblocks,
    unsigned int blockalign)
{
    if (!(blockallocsize * numblocks))
        return -1;

    WaitForSingleObject(s_mutexSlots[s_memMutex].handle, INFINITE);

    // Find a free memgroup slot
    int slotIdx = 0;
    memgroupdef_s* slot = memgroups;
    while (slot->used)
    {
        ++slot;
        ++slotIdx;
        if (slot >= (memgroupdef_s*)&testMtx)
        {
            ReleaseMutex(s_mutexSlots[s_memMutex].handle);
            return -1;
        }
    }

    // Align block size up
    if (blockalign < 4) blockalign = 4;
    int alignedBlockSize = (blockallocsize + blockalign - 1) & ~(int)(blockalign - 1);

    // Store name
    vafmtbuff(slot->name, 16, "%s", inName);

    // Allocate the block pool memory
    char* mem = (char*)memAllocAlignHighCore(
        alignedBlockSize * numblocks, 0, blockalign,
        "source/mem/mem.cpp", 1055, inName, 1);

    // Fill in group descriptor
    slot->flags = 0;
    slot->groupMutex = -1;
    slot->used = 1;
    slot->type = MEMGROUP_FIXEDSIZE;
    slot->assertonfail = 1;
    slot->groupstart = mem;
    slot->groupend = mem + alignedBlockSize * numblocks;

    // Init the pool
    slot->___u9.fixed.pool.freeCount = 0;
    slot->___u9.fixed.pool.totalCount = 0;
    slot->___u9.fixed.pool.objectSize = alignedBlockSize & 0x0FFFFFFF;
    slot->___u9.fixed.pool.free = nullptr;
    slot->___u9.fixed.pool.objects = mem;

    poolAddObjectsArray(&slot->___u9.fixed.pool, mem, numblocks);

    ReleaseMutex(s_mutexSlots[s_memMutex].handle);
    return slotIdx;
}

void queueInit(queueState_s* q, unsigned int itemCount, unsigned int itemSize)
{
    q->maxSize = itemCount;
    q->count = 0;
    q->front = 0;

    if ((int)(itemCount * itemSize) > 0)
        q->items = (unsigned __int8*)memAllocAlignCore(
            itemCount * itemSize, 0, 0, __FILE__, 10, nullptr, 1);
    else
        q->items = nullptr;
}

void poolInitFromHeap(poolState* pool, unsigned int heap, unsigned int objectSize, unsigned int objectArrayLength)
{
    void* v7 = nullptr;
    int   total = objectSize * objectArrayLength;

    if (total > 0)
        v7 = memAllocAlignCore(total, heap, 0, __FILE__, 103, nullptr, 1);

    pool->objects = v7;
    pool->freeCount = 0;
    pool->totalCount = 0;
    pool->free = nullptr;

    // Store objectSize with upper nibble masked off
    ((DWORD*)pool)[4] = objectSize & 0x0FFFFFFF;

    poolAddObjectsArray(pool, (char*)v7, objectArrayLength);

    // Set the 0x10000000 flag to indicate pool owns its memory
    ((DWORD*)pool)[4] |= 0x10000000;
}

void listInitFromHeap(listState_s* list, unsigned int heap, unsigned int itemCount, unsigned int itemSize)
{
    unsigned __int8* v9 = nullptr;
    unsigned int     v5 = itemCount;

    if (itemCount)
    {
        char* comment = evafmt("listInitFromHeap(%d, %d)", itemCount, itemSize);

        int total = itemCount * itemSize;
        if (total > 0)
            v9 = (unsigned __int8*)memAllocAlignCore(
                total, heap, 0, "source/util/list.c", 16, comment, 1);

        if (!v9)
            v5 = 0;
    }

    list->maxSize = v5;
    list->items = v9;
    list->flags = 1;
    list->count = 0;
}