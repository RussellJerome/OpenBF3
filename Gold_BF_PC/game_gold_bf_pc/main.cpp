// game_gold_bf_pc.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <windows.h>
#include "engine/engineinit.h"
#include "engine/mem.h"
#include "engine/string/engstring.h"
#include "Logger/Log.h"
#include "util/mathf.h"
#include "file/packfile.h"
#include "util/unorgtypes.h"
#include "dictionary/CEmbeddedResFile.h"
#include "util/powerpc.h"

#include "framework/template/CTemplateMgr.h"

#include "engine/platform_pc/WindowsWindow.h"

#include "engine/Font.h"

#include "engine/platform_pc/hw.h"

#include "engine/bg/bg.h"

#include "engine/scene/scene.h"

#include "framework/network/FRDPeer.h"

struct HOM;


typedef void (*EngineWorldFunc)();

struct WorldStateData
{
    //temp
    int              WorldState;
    BYTE               s_remakeAllMissingAssets;
    char               s_buildInfo[0x100];
    EngineWorldFunc    engineInitialiseWorldFunc;
    EngineWorldFunc    engineDeinitialiseWorldFunc;
    EngineWorldFunc    engineTickWorldFunc;
    EngineWorldFunc    enginePhysicsBeginTick;
    EngineWorldFunc    engineWaitForPhysicsEnd;
    void* s_engineQuitFunc;
};

WorldStateData WorldState = {};
const char* g_buildStuff = nullptr;
const char* g_buildStuffForDisplay = nullptr;

_LARGE_INTEGER timerGameStartTime = { { 0, 0u } };
_LARGE_INTEGER timerFrequency = { { 0, 0u } };
void timerTick(bool paused);
int bossInitialise()
{
    DbgPrint("bossInitialise");
    // One-time initialise mutex slot table
    if (!mutexIsInitialised)
    {
        memset(s_mutexSlots, 0, sizeof(s_mutexSlots));
        mutexIsInitialised = 1;
    }

    // Zero all condition variable slots
    memset(s_conditionSlots, 0, sizeof(s_conditionSlots));

    // Query timer frequency and capture start time
    QueryPerformanceFrequency(&timerFrequency);
    QueryPerformanceCounter(&timerGameStartTime);

    // Initialise timer state
    timerGameFrameNum = 0;
    timerGameTime = 0.0;
    timerLastGameFrameDuration = 0.0f;
    timerTargetGameTime = 0.0;

    return 0;
}

char g_project[0x20];


char s_remoteSharePath[128] = {};
static const char s_fopenModeR[] = "r";
static const char s_pathSeparator[] = "\\";
static const char aNetSmb[] = "net:\\smb\\";
static const char s_hwPathFallback[] = "d:\\";


bool fileSetXenonShare(const char* xenonShareFilename, char* remoteSharePath)
{
    DbgPrint("fileSetXenonShare");
    char buf[255];
    memset(buf, 0, sizeof(buf));

    FILE* f = fopen(xenonShareFilename, "r");
    if (!f)
        return false;

    if (fread(buf, 1, 255, f))
    {
        // Strip trailing newline if present
        int len = (int)strlen(buf);
        if (buf[len - 1] == '\n')
            buf[len - 1] = '\0';

        // Build remote share path: "net:\\smb\\" + buf + "\\"
        strcpy(remoteSharePath, aNetSmb);
        strcat(remoteSharePath, buf);
        strcat(remoteSharePath, s_pathSeparator);
        DbgPrint(remoteSharePath);
        // Validate the path exists
        if (GetFileAttributesA(remoteSharePath) == (DWORD)-1)
        {
            GetLastError();
            remoteSharePath[0] = '\0';
        }
    }

    fclose(f);

    // Return true if remoteSharePath is non-empty
    return remoteSharePath[0] != '\0';
}


static char s_hwPathBuffer[MAX_PATH];

__int64 synchronizationTimeDelta = 0;



void packfilesInit()
{
    DbgPrint("packfilesInit");
    // Zero entire packfile table
    memset(s_packfiles, 0, sizeof(s_packfiles));

    // Explicitly zero status fields on each slot
    // (belt-and-suspenders after XMemSet — m_inmem, m_used, m_initialised,
    //  m_needsClose and m_refCount are all cleared)
    for (int i = 0; i < 16; i++)
    {
        s_packfiles[i].m_inmem = 0;
        s_packfiles[i].m_used = 0;
        s_packfiles[i].m_initialised = 0;
        s_packfiles[i].m_needsClose = 0;
        s_packfiles[i].m_refCount = 0;
    }

    // Open filesystem pack into the first free slot
    DbgPrint("Trying pak/xenon/fsys.pak");
    int fsysResult = packfileOpenAtIdx("pak\\xenon\\fsys.pak", 0, findFreePackfileSlot());
    DbgPrint("fsys.pak open result: %d", fsysResult);

    // Open shader pack into the next free slot
    DbgPrint("Trying pak/xenon/shader.pak");
    packfileOpenAtIdx("pak\\xenon\\shader.pak", 0, findFreePackfileSlot());
}


void fileTableInit()
{
    // Zero the entire pool data
    memset(s_dirPoolData, 0, sizeof(s_dirPoolData));

    // Initialise the pool descriptor
    s_dirPool.objects = s_dirPoolData;
    s_dirPool.free = nullptr;
    s_dirPool.freeCount = 0;
    s_dirPool.totalCount = 0;
    s_dirPool.objectSize = sizeof(TDirEntry);  // 96 = 0x60

    // Build the free list — thread each entry's first DWORD to point to
    // the previous entry, walking backward from the last slot
    poolObject* prev = nullptr;
    for (int i = 14999; i >= 0; i--)
    {
        poolObject* slot = (poolObject*)&s_dirPoolData[i];
        slot->next = prev;
        prev = slot;
    }

    s_dirPool.free = prev;           // head of free list = first slot
    s_dirPool.freeCount += 15000;
    s_dirPool.totalCount += 15000;

    // If pool data wasn't allocated inline (external allocation), set flag
    if (s_dirPoolData != s_dirPool.objects)
        s_dirPool.objectSize |= 0x40000000u;

    // Zero the root directory entry
    memset(&s_root, 0, sizeof(TDirEntry));
}

void dumpPakContents(packfileinfo* slot)
{
    /*if (!slot || !slot->m_used)
    {
        DbgPrint("Pak not loaded\n");
        return;
    }

    unsigned int ftsize = slot->m_pf->m_ftsize;
    DbgPrint("=== PAK DUMP: %s (%u files) ===\n", slot->m_name, ftsize);

    for (unsigned int i = 0; i < ftsize; i++)
    {
        if (slot->m_st && slot->m_sto)
        {
            const char* name = slot->m_st + slot->m_sto[i];
            DbgPrint("  [%u] %s (size=%u packed=%u chunk=%u)\n",
                i, name,
                slot->m_ft[i].m_filesize,
                slot->m_ft[i].m_packedsize,
                slot->m_ft[i].m_chunk);
        }
        else
        {
            DbgPrint("  [%u] hash=0x%08X size=%u\n",
                i, slot->m_ht[i], slot->m_ft[i].m_filesize);
        }
    }

    DbgPrint("=== END PAK DUMP ===\n");*/
}


void fileInitialise()
{
    DbgPrint("fileInitialise");
    // Initialise file info slots — set m_fh and m_pack to -1, leave rest zeroed
    for (int i = 0; i < 64; i++)
    {
        s_fileInfos[i].m_fh = (unsigned int)-1;
        s_fileInfos[i].m_pack = -1;
    }

    // Zero file handle table
    memset(s_fileHandles, 0, sizeof(s_fileHandles));

    // Try to mount xenon share — if successful, use remote path as hw path
    if (fileSetXenonShare("c:\\xenonShare.txt", s_remoteSharePath))
    {
        s_hwPath = s_remoteSharePath;
        DbgPrint("fileSetXenonShare FOUND");
    }
    else
    {
        //s_hwPath = s_hwPathFallback;   // "d:\\"

        //Cheap PC change so we dont hard code a path. Will redo
        char cpath[256];
        GetModuleFileNameA(NULL, cpath, MAX_PATH);
        std::string path = cpath;
        path = path.substr(0, path.find_last_of("/\\"));

        //Clean Folder
        path = path + "\\Game\\";

        strncpy_s(s_hwPathBuffer, path.c_str(), _TRUNCATE);
        s_hwPath = s_hwPathBuffer;

    }
        
    packfilesInit();
    for (int i = 0; i < 16; i++)
    {
        if (s_packfiles[i].m_used)
        {
            dumpPakContents(&s_packfiles[i]);
        }
            
    }
    fileTableInit();

    // If cache is enabled but sync is also enabled, disable cache
    // If cache is enabled and sync is disabled, zero the sync delta
    if (cacheEnabled)
    {
        if (cacheSyncEnabled)
            cacheEnabled = 0;
        else
            synchronizationTimeDelta = 0;
    }
}

BYTE  g_usingStage = 0;  // .data:82D0FE region
BYTE  s_usingEditor = 0;
BYTE s_enableRuntimeEmbed = 0;

void checkCommandLineArgumentsEarly(int argc, char** argv)
{
    if (argc <= 1)
        return;

    for (int i = 1; i < argc; i++)
    {
        const char* arg = argv[i];

        if (strcmp(arg, "-editor") == 0)
        {
            s_usingEditor = 1;
        }
        else if (strcmp(arg, "-stage") == 0)
        {
            g_usingStage = 1;
        }
        else if (strcmp(arg, "-noembed") == 0)
        {
            s_enableRuntimeEmbed = 0;
        }
        else if (strcmp(arg, "-embed") == 0)
        {
            s_enableRuntimeEmbed = 1;
        }
    }
}

unsigned int _S1_19 = 0u;


CTemplateMgr* s_tmpMgr = nullptr;


unsigned __int8* relocateResourceGold(unsigned __int8* resource)
{
    // Read top-level offsets BEFORE any writes
    unsigned int topCount = bswap32(((unsigned int*)resource)[2]);
    unsigned int topOffsets[16] = {};
    for (unsigned int i = 0; i < topCount && i < 16; i++)
        topOffsets[i] = bswap32(((unsigned int*)resource)[3 + i]);

    // Apply top-level relocations
    for (unsigned int i = 0; i < topCount; i++)
    {
        unsigned int ptrOffset = topOffsets[i];
        unsigned int* ptrSlot = (unsigned int*)(resource + ptrOffset);
        unsigned int storedOffset = bswap32(*ptrSlot);
        if (storedOffset != 0)
            *ptrSlot = (unsigned int)(resource + storedOffset);
        else
            *ptrSlot = 0;
    }

    // Large relocation table - skip any offsets already handled above
    unsigned int relocOffset = bswap32(((unsigned int*)resource)[0]);
    unsigned int* relocHdr = (unsigned int*)(resource + relocOffset);
    unsigned int count = bswap32(relocHdr[0]);
    unsigned int* offsets = &relocHdr[1];

    for (unsigned int i = 0; i < count; i++)
    {
        unsigned int ptrOffset = bswap32(offsets[i]);

        // Skip if already relocated by top-level pass
        bool alreadyDone = false;
        for (unsigned int j = 0; j < topCount; j++)
        {
            if (ptrOffset == topOffsets[j])
            {
                alreadyDone = true;
                break;
            }
        }
        if (alreadyDone) continue;

        unsigned int* ptrSlot = (unsigned int*)(resource + ptrOffset);
        unsigned int storedOffset = bswap32(*ptrSlot);
        if (storedOffset != 0)
            *ptrSlot = (unsigned int)(resource + storedOffset);
        else
            *ptrSlot = 0;
    }

    return resource + 0x10;
}

CHashStrPool* s_strPool = NULL;

void DebugDumpSettings(CDictionary* dict)
{
    if (!dict) { DbgPrint("g_settings is NULL"); return; }
    CHashDictionary* hdict = (CHashDictionary*)dict;

    SHashEntry* entry = hdict->m_contentsHead;
    int count = 0;
    while (entry)
    {
        const char* key = *(const char**)((char*)entry + 0x00);
        if (key)
        {
            unsigned char type = 0;
            TDictDataValue value = {};
            unsigned short flags = 0;
            if (dict->GetOptionalValue(key, &type, &value, &flags))
            {
                if (type == 4)
                    DbgPrint("settings[%d]: '%s' = '%s' (string)", count, key, value.s ? value.s : "<null>");
                else if (type == 1)
                    DbgPrint("settings[%d]: '%s' = %d (int)", count, key, value.i);
                else if (type == 8)
                    DbgPrint("settings[%d]: '%s' = <dict> (type 8)", count, key);
                else
                    DbgPrint("settings[%d]: '%s' = <type %d>", count, key, type);
            }
            else
            {
                DbgPrint("settings[%d]: '%s' = <not found via getter>", count, key);
            }
        }
        entry = *(SHashEntry**)((char*)entry + 0x1C);
        count++;
        if (count > 100) { DbgPrint("...truncated"); break; }
    }
    DbgPrint("Total: %d entries", count);
}

CDictionary* VehicleTest;
void TestResLoad(const char* inVehicleFile)
{
    // Destroy existing settings if any
    if (VehicleTest)
    {
        delete VehicleTest;
        VehicleTest = nullptr;
    }

    // Reuse or create string pool
    if (s_strPool)
    {
        s_strPool->Clear();
    }
    else
    {
        void* mem = memAllocAlignCore(
            0xA4Cu, 0, 0,
            "d:/user/nightly/70192/ms_mar_08/engine/include\\engine/boss/common.h",
            589,
            "memalloc no group",
            2);
        if (mem)
            s_strPool = new (mem) CHashStrPool(-1);
        else
            s_strPool = nullptr;
    }

    // Parse settings file
    CDictionaryFileParser parser(s_strPool, s_strPool, 1, nullptr);

    if (parser.PushInputFile(inVehicleFile))
    {
        parser.m_includeListStrs = nullptr;
        parser.m_includeList = nullptr;
        DbgPrint("parser this = %p, m_flags = %d", &parser, parser.m_flags);

        VehicleTest = parser.DoParse();

        DebugDumpSettings((CHashDictionary*)VehicleTest);
        std::cout << "VehicleTest" << std::endl;
    }
}

void settingsInit(const char* inSettingsFile)
{
    // Destroy existing settings if any
    if (g_settings)
    {
        delete g_settings;
        g_settings = nullptr;
    }

    // Reuse or create string pool
    if (s_strPool)
    {
        s_strPool->Clear();
    }
    else
    {
        void* mem = memAllocAlignCore(
            0xA4Cu, 0, 0,
            "d:/user/nightly/70192/ms_mar_08/engine/include\\engine/boss/common.h",
            589,
            "memalloc no group",
            2);
        if (mem)
            s_strPool = new (mem) CHashStrPool(-1);
        else
            s_strPool = nullptr;
    }

    // Parse settings file
    /*CDictionaryFileParser parser(s_strPool, s_strPool, 1, nullptr);

    if (parser.PushInputFile(inSettingsFile))
    {
        parser.m_includeListStrs = nullptr;
        parser.m_includeList = nullptr;
        DbgPrint("parser this = %p, m_flags = %d", &parser, parser.m_flags);

        g_settings = parser.DoParse();

        DebugDumpSettings((CHashDictionary*)g_settings);

        std::cout << "ParseCast" << std::endl;
    }*/

    g_settings = new settings_res();
}

void templatesInit()
{
    if ((_S1_19 & 1) == 0)
    {
        _S1_19 |= 1u;
        s_keyPool_2 = new CHashStrPool(-1);
    }

    unsigned __int8* mem = (unsigned __int8*)memAllocAlignCore(
        0x8D9C0u, 0, 0,
        "d:/user/nightly/70192/ms_mar_08/engine/include\\engine/boss/common.h",
        589,
        "memalloc no group",
        2);

    CTemplateMgr* tmpMgr = nullptr;
    if (mem)
        tmpMgr = new (mem) CTemplateMgr(s_keyPool_2);
    else
        tmpMgr = nullptr;

    s_tmpMgr = tmpMgr;
    
    StVafmtT<512> v21("assets\\%s\\embed_xb_v%d\\data\\%s\\templates.war",
        g_project, 4, g_project);

    if (!s_enableRuntimeEmbed)
        CEmbeddedResFile::m_allowRuntimeEmbed = 0;

    unsigned int v5 = fileSize(v21.str);

    unsigned __int8* v6 = (unsigned __int8*)memAllocAlignHighCore(
        v5, 0, 16,
        "d:/user/nightly/70192/ms_mar_08/game\\source/common/main/main.cpp",
        588,
        v21.str,
        1u);

    unsigned int v7 = fileOpen(v21.str, 1);
    fileRead(v7, v6, v5);
    fileClose(v7);

    const SEmbeddedResFileHeader* v8 = (const SEmbeddedResFileHeader*)relocateResourceGold(v6);
    // Byte-swap all integer fields in place
    SEmbeddedResFileHeader* v8mut = (SEmbeddedResFileHeader*)v8;
    v8mut->resFileDataSize = bswap32(v8mut->resFileDataSize);
    v8mut->keyPoolSize = bswap32(v8mut->keyPoolSize);
    v8mut->strValuePoolSize = bswap32(v8mut->strValuePoolSize);
    v8mut->numEntries = bswap32(v8mut->numEntries);
    v8mut->numTemplates = bswap32(v8mut->numTemplates);
    v8mut->preloadListSize = bswap32(v8mut->preloadListSize);
    v8mut->animInfoOffset = bswap32(v8mut->animInfoOffset);

    // Byte-swap template offsets
    for (int i = 0; i < v8->numTemplates; i++)
    {
        v8mut->templates[i].offset = bswap32(v8mut->templates[i].offset);
        const char* name = v8mut->templates[i].name;
    }

    // Dump key pool
    /*const char* keyPool = (const char*)v8->keyPool;
    int keyPoolEnd = (int)bswap32((unsigned int)v8->keyPoolSize);
    DbgPrint("=== KEY POOL DUMP ===");
    int pos = 0;
    int count = 0;
    while (pos < keyPoolEnd)
    {
        const char* key = keyPool + pos;
        int len = (int)strlen(key);
        if (len > 0)
        {
            DbgPrint("  [%d] %s", count++, key);
            pos += len + 1;
        }
        else
        {
            pos++;
        }
    }
    DbgPrint("=== END KEY POOL (%d keys) ===", count);*/

    poolObject* free = s_embeddedResFilePool.free;
    if (s_embeddedResFilePool.free)
    {
        s_embeddedResFilePool.free = s_embeddedResFilePool.free->next;
        --s_embeddedResFilePool.freeCount;
    }

    CEmbeddedResFile* v11 = nullptr;
    if (free)
    {
        v11 = new (free) CEmbeddedResFile(v8, nullptr);
    }
    else
    {
        //The game didnt have this
        // Pool empty — allocate directly from game heap
        void* mem = memAllocAlignCore(sizeof(CEmbeddedResFile), 0, 16,
            "source/common/main/main.cpp", 0, "", 1);
        if (mem)
            v11 = new (mem) CEmbeddedResFile(v8, nullptr);
    }

    if (v8->numTemplates > 0)
    {
        for (int i = 0; i < v8->numTemplates; i++)
        {
            CTemplateMgr::s_mgr->AddTemplate(
                v8->templates[i].name,
                (CDictionary*)(v8->templates[i].offset + *(int*)v11));
        }
    }

    memFreeFlags((char*)v6, 1u);
}

unsigned __int8 s_assertPrintfsEnabled = 1u;
int s_defaultTickFrequency = 25;

//Being Reworked
void settingsProcessKeyValue(const char* inKey, char* inVal)
{
    //char* endptr = nullptr;
    //int ibase = 10;

    //// Check for hex prefix "0x"
    //int len = (int)strlen(inVal);
    //if (len > 2 && inVal[0] == '0' && inVal[1] == 'x')
    //    ibase = 16;

    //// Try integer first
    //long iVal = strtol(inVal, &endptr, ibase);
    //if (*endptr == 0)
    //{
    //    // Pure integer
    //    TDictDataValue dv = {};
    //    dv.i = (int)iVal;
    //    g_settings->EnterValueWithSourceLocation(inKey, 1, &dv, 0, nullptr, 0);
    //    return;
    //}

    //// Try float
    //double dVal = strtod(inVal, &endptr);
    //if (*endptr == 0)
    //{
    //    // Pure float
    //    TDictDataValue dv = {};
    //    float f = (float)dVal;
    //    memcpy(&dv.i, &f, 4);
    //    g_settings->EnterValueWithSourceLocation(inKey, 2, &dv, 0, nullptr, 0);
    //    return;
    //}

    //// String — intern via s_strPool->Copy
    //const char* interned = s_strPool->Copy(inVal);
    //TDictDataValue dv = {};
    //dv.s = interned;
    //g_settings->EnterValueWithSourceLocation(inKey, 4, &dv, 0, nullptr, 0);
}

void mainInitProjectSettings(int argc, char** argv)
{
    templatesInit();

    // Build path to the project settings file: "data/<project>/settings.res"
    char settingsPath[512];
    vafmtbuff(settingsPath, sizeof(settingsPath), "data/%s/settings.res", g_project);

    // Temporarily allow file access to check if the settings file exists
    BYTE savedAccess = s_fileAccessAllowed;
    s_fileAccessAllowed = 1;
    unsigned int size = fileSize(settingsPath);
    s_fileAccessAllowed = savedAccess;

    if (size == (unsigned int)-1)
        exit(1);

    settingsInit(settingsPath);
    // Process any "key=value" command-line arguments
    for (int i = 1; i < argc; i++)
    {
        char* arg = argv[i];
        char* equals = strchr(arg, '=');
        if (equals)
        {
            int keyLen = (int)(equals - arg);
            if (keyLen < 512)
            {
                char key[512];
                memcpy(key, arg, keyLen);
                key[keyLen] = '\0';
                settingsProcessKeyValue(key, equals + 1);
            }
        }
    }

    // Apply settings from the dictionary
    bool disableAsserts = g_settings->disableasserts;
    //g_settings->GetOptionalBoolValue("disableasserts", &disableAsserts, false);

    if (disableAsserts)
    {
        bool stillPrint = g_settings->disabledAssertsStillPrint;
        //g_settings->GetOptionalBoolValue("disabledAssertsStillPrint",&stillPrint, false);
        g_assertsEnabled = 0;
        s_assertPrintfsEnabled = stillPrint ? 1 : 0;
    }

    TDictDataValue texstreamtickfreq;
    //if (g_settings->GetOptionalIntValue("texstreamtickfreq", &texstreamtickfreq, 0))
    texstreamtickfreq.i = g_settings->texstreamtickfreq;
    if(&texstreamtickfreq)
    {
        s_defaultTickFrequency = (unsigned int)texstreamtickfreq.i;

        std::cout << "texstreamtickfreq: " << s_defaultTickFrequency << std::endl;
    }

    // memdbglevel — read value but no action taken (debug/logging only)
    const char* memDbgLevel = g_settings->memdbglevel;
    //if (g_settings->GetOptionalStringValue("memdbglevel", &memDbgLevel))
    if(memDbgLevel)
    {
        _stricmp(memDbgLevel, "low");
        _stricmp(memDbgLevel, "off");
        _stricmp(memDbgLevel, "high");
    }
}

void dictionaryInitialise(
    int inDictContentSize,
    int inHashDictionaryNumber,
    int inPackedDictionaryNumber,
    int inFixedSizeDictionarySize,
    int inFixedSizeDictionaryNumber,
    int inFixedSizeArraySize,
    int inFixedSizeArrayNumber)
{
    CPackedStructure::s_objectHeap = memAllocGroupFixedBlockSize(
        "packedDictObs", 64, inPackedDictionaryNumber, 4);

    CDictionary::s_heapDictContents = memAllocGroupFixedBlockSize(
        "dictionary", 0x104, inDictContentSize / 0x104, 4);

    CHashDictionary::s_objectHeap = memAllocGroupFixedBlockSize(
        "hashDictionary", 0x25C, inHashDictionaryNumber, 4);

    //CFixedSizeDictionaryCommon::s_objectHeap = memAllocGroupFixedBlockSize(
    //    "fixedSizeDict", inFixedSizeDictionarySize, inFixedSizeDictionaryNumber, 4);

    //CFixedSizeArrayCommon::s_objectHeap = memAllocGroupFixedBlockSize(
    //    "fixedSizeArray", inFixedSizeArraySize, inFixedSizeArrayNumber, 4);

    void* pool = memAllocAlignCore(
        0x1DB0, 0, 0,
        "source/dictionary/CEmbeddedResFile.cpp", 0x1A7, "", 1);

    s_embeddedResFilePool.freeCount = 0;
    s_embeddedResFilePool.totalCount = 0;
    s_embeddedResFilePool.objectSize = 0x4C;
    s_embeddedResFilePool.free = nullptr;
    s_embeddedResFilePool.objects = pool;
    poolAddObjectsArray(&s_embeddedResFilePool, (char*)pool, 0x64);
}


//WRAPPED FUNCS
struct EngineWorldData
{
    int numobs;
    int numprops;
};

EngineWorldData worldData;
int engineframe = 0;
float lasttimes[10];
int lasttimescount;
float framerate;
float avgFramerate;
float avgFramerate1;

void status_updateFrameRateGOLD()
{
    lasttimes[lasttimescount] = timerLastFrameDuration;
    lasttimescount = (lasttimescount + 1) % 10;

    if (lasttimescount == 0)
    {
        float sum = 0.0f;
        for (int i = 0; i < 10; i++)
            sum += lasttimes[i];

        float avg = sum * 0.1f;

        float fps;
        if (avg <= 0.0f)
        {
            fps = framerate;
        }
        else
        {
            fps = 1.0f / avg;
            framerate = fps;
        }

        if (fps > 1.0f)
        {
            avgFramerate = (avgFramerate * 9.0f + fps) * 0.1f;
            avgFramerate1 = (avgFramerate1 * 9.0f + avgFramerate) * 0.1f;
        }
    }
}

int count_3 = 0;
void status_displayFrameRateGOLD()
{
    statusProject();

    // Set font size commands
    int cmd[2];
    cmd[0] = 30;
    AddFontCommand(cmd, 4);
    cmd[0] = 20;
    AddFontCommand(cmd, 4);

    fontInitialiseState(&fontState);

    // Load and set font
    int fontHandle = fontFindLoadedFont("book");
    if (fontHandle != -1)
        fontSet(fontHandle);

    // Set up font window
    fontSetFixed(10.0f);
    float h = fontGetHeight();
    fontSetWindow(
        0.02f,
        1.0f - (h * 2.0f) - 0.05f,
        0.96f,
        h * 2.0f);

    fontSetFlag(8);

    // Set build strings
    g_buildStuff = "FRDBVERSFRDSbranches/bf/ms_mar_08 70217 GOLD BUILDFRDBVERSFRDE";
    g_buildStuffForDisplay = "branches/bf/ms_mar_08 70217 GOLD BUILD";

    // Format and print framerate string
    char buf[256];
    vafmtbuff(buf, 256, "Buildnum: r%s    %.2f fps\n", "70217", framerate);

    fontSetAlignment(FONT_ALIGN_RIGHT);
    fontPrint(buf);
    fontSetAlignment(FONT_ALIGN_LEFT);

    // Restore font commands
    cmd[0] = 18;
    cmd[1] = 8;
    AddFontCommand(cmd, 8);

    fontState.___u0.__s0.flag &= ~8u;

    cmd[0] = 31;
    AddFontCommand(cmd, 4);

    count_3 = (count_3 + 1) % 4;
}

//void engineInitialise(EngineInitData* data)
//{
//    WinWindows::GameWindow = new WinWindows(0, data->screenResX, data->screenResY);
//    texHwInit();
//    if (!WinWindows::GameWindow->InitWindow())
//    {
//        return;
//    }
//    InitD3D();
//    worldData.numobs = 256;
//    worldData.numprops = 1500;
//    engineframe = 0;
//}

struct engine_globals
{
    obHandleDef obhandles[1050];
};

float SinTable[2048];
unsigned int navTaskIDs[20];
engine_globals g;
electricLine_s* sLines = NULL; // idb
electricSphere_s* sSpheres = NULL; // idb
lightning_s* sLightningPool = NULL;
unsigned int s_defaultTexture;
unsigned int s_textureChroma = 4294967295u;
unsigned int s_textureRainMap = 4294967295u;
unsigned int s_textureWhiteNoise = 4294967295u;
unsigned int s_textureFont = 4294967295u;
int s_numBlurPasses = 0;
obStreamDef s_obStream[3];
unsigned int currentH = 0u;
slinklistdef_s obinstListDelete;
unsigned int* s_glows = NULL;
unsigned int* s_drawGlows = NULL;
unsigned __int8 s_menusTerminated = 1u;
MenuHead_s menus[70];
BYTE algn_82D85A98[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
slinklistdef_s scenelist = { NULL, 0 };
int prevCur = 0;
unsigned __int8 justClosed = 0u;
flare_s* s_flares = NULL;
vec3_u* s_sphere = NULL;
int s_flareNum = 0;
int s_explosionNum = 0;
explosion_s* s_explosions = NULL;

unsigned int s_obindexcount = 4294967295u;
unsigned int s_obvertexcount = 4294967295u;
IDirect3DIndexBuffer9* s_pObIndexBuffers = NULL;
IDirect3DIndexBuffer9** s_pObIndexBuffersAvailibility = NULL;
IDirect3DVertexBuffer9** s_pObVertexBuffersAvailibility = NULL;
IDirect3DVertexBuffer9* s_pObVertexBuffers = NULL;
IDirect3DVertexBuffer9* s_pObSkinningVertexBuffers = NULL;
int s_obHwMemGroup = 0;
int obInitialiseHW()
{
    s_obindexcount = 0;
    s_obvertexcount = 0;

    // Allocate index buffer pool (0x10000 bytes = 65536 bytes)
    s_pObIndexBuffers = (IDirect3DIndexBuffer9*)memAllocAlignCore(
        0x10000, s_obHwMemGroup, 0, __FILE__, 136, nullptr, 1);
    memset(s_pObIndexBuffers, 0, 0x10000);

    // Allocate index buffer availability list (2048 entries)
    IDirect3DIndexBuffer9** v0 = (IDirect3DIndexBuffer9**)memAllocAlignCore(
        0x2000, s_obHwMemGroup, 0, __FILE__, 140, nullptr, 1);
    s_pObIndexBuffersAvailibility = v0;

    IDirect3DIndexBuffer9* v1 = s_pObIndexBuffers;
    for (int v2 = 2048; v2 > 0; --v2)
        *v0++ = v1++;

    // Allocate vertex buffer pool (0x13880 bytes)
    s_pObVertexBuffers = (IDirect3DVertexBuffer9*)memAllocAlignCore(
        0x13880, s_obHwMemGroup, 0, __FILE__, 147, nullptr, 1);
    memset(s_pObVertexBuffers, 0, 0x13880);

    // Allocate vertex buffer availability list (2500 entries)
    IDirect3DVertexBuffer9** v3 = (IDirect3DVertexBuffer9**)memAllocAlignCore(
        0x2710, s_obHwMemGroup, 0, __FILE__, 151, nullptr, 1);
    s_pObVertexBuffersAvailibility = v3;

    IDirect3DVertexBuffer9* v4 = s_pObVertexBuffers;
    for (int v5 = 2500; v5 > 0; --v5)
        *v3++ = v4++;

    // Allocate skinning vertex buffer pool (0x8000 bytes)
    s_pObSkinningVertexBuffers = (IDirect3DVertexBuffer9*)memAllocAlignCore(
        0x8000, s_obHwMemGroup, 0, __FILE__, 158, nullptr, 1);
    memset(s_pObSkinningVertexBuffers, 0, 0x8000);

    return 1;
}

unsigned __int8 g_streamShaders = 0u;
queueState_s s_bgStreamQueue = { 0u, 0u, 0u, NULL };
poolState objpool;

void glowReset()
{
    memset(s_drawGlows, 0, 0x40);
    memset(s_glows, 0, 0x200);

    // Initialize first 16 entries of both arrays to -1
    unsigned int v3 = 0;
    do
    {
        s_drawGlows[v3] = -1;
        s_glows[v3] = -1;
        v3 = (unsigned __int8)(v3 + 1);
    } while (v3 < 0x10);

    // Initialize entries 16-127 of s_glows to -1
    unsigned int v5 = 16;
    do
    {
        s_glows[v5] = -1;
        v5 = (unsigned __int8)(v5 + 1);
    } while (v5 < 0x80);
}

void engineInitialise(EngineInitData* data)
{
    WinWindows::GameWindow = new WinWindows(0, data->screenResX, data->screenResY);

    if (!WinWindows::GameWindow->InitWindow())
    {
        return;
    }

    // Build sine table
    int i = 0;
    float* p = SinTable;
    do
    {
        *p++ = sinf((float)i * 0.00048828125f * 6.2831855f);
        i++;
    } while ((int)p < (int)navTaskIDs);

    // Init global object array
    memset(&g, 0, 0x6270);
    unsigned int* ftHandle = &g.obhandles[0].ftHandle;
    do
    {
        *ftHandle = (unsigned int)-1;
        ftHandle += 6;
    } while ((int)ftHandle < (int)&g.obhandles[ARRAYSIZE(g.obhandles)]);

    memcpy(&initData, data, sizeof(EngineInitData));

    if (initData.memInitData.heap)
    {
        fileInitialise();
        memInitialise(initData.memInitData.heap, initData.memInitData.heapSize);
        dictionaryInitialise(
            initData.memInitData.dictContentsHeapSize,
            initData.memInitData.hashDictHeapNumber,
            initData.memInitData.packedDictionaryNumber,
            initData.memInitData.fixedSizeDictHeapSize,
            initData.memInitData.fixedSizeDictHeapNumber,
            initData.memInitData.fixedSizeArrayHeapSize,
            initData.memInitData.fixedSizeArrayHeapNumber);
    }

    hwInitialise();
    //shaderLoadCfg4();

    windowfullscreen.pixelaspect = initData.pixelAspect;

    //DL_TEXTURE_FRAME = (unsigned int)-1;
    //DL_TEXTURE_FRAME_SCRATCH = (unsigned int)-1;
    //DL_TEXTURE_FRAME_QUARTER = (unsigned int)-1;
    //DL_TEXTURE_FRAME_LAST = (unsigned int)-1;
    //DL_TEXTURE_DEPTH = (unsigned int)-1;

    PBuffer_s* pb = pbuffers;
    do
    {
        pb->valid = 0;
        pb++;
    } while ((int)pb < (int)&s_wodge);

    //pbufferInitialiseHW();
    //hdrInitialiseRenderTargets();
    //obShadowBufferInitialise();

    //shaderTargetList targetList = {};
    //targetList.targetCount = 0;
    //createPbufferPasses(shaderContext.passes, &targetList);

    //for (ShaderTreePass* pass = shaderContext.tree; pass; pass = pass->next)
    //    for (ShaderTreeMaterial* mat = pass->material; mat; mat = mat->next)
    //        for (ShaderTreeIteration* iter = mat->iteration; iter; iter = iter->next)
    //            if (iter->target)
    //                shaderSetupTarget(iter->target);

    //pmeshInitialise();
    //terrainWaterInitialise();

    if (!sLines)
        sLines = (electricLine_s*)memAllocAlignCore(179456, 0, 0, "source/renderer/electricgfx.c", 649, "", 1);

    if (!sSpheres)
        sSpheres = (electricSphere_s*)memAllocAlignCore(3008, 0, 0, "source/renderer/electricgfx.c", 653, "", 1);

    cloudInitialise();

    if (!sLightningPool)
        sLightningPool = (lightning_s*)memAllocAlignCore(862720, 0, 0, "source/renderer/lightning.c", 110, "", 1);

    s_textureChroma = (unsigned int)-1;
    s_textureFont = (unsigned int)-1;
    s_textureRainMap = (unsigned int)-1;
    s_textureWhiteNoise = (unsigned int)-1;
    //dword_82CCE37C = (unsigned int)-1;
    sTextureCircleSun = (unsigned int)-1;
    //dword_82CB74A4 = (unsigned int)-1;
    //dword_82CB749C = (unsigned int)-1;
    //dword_82CB74A8 = (unsigned int)-1;
    //dword_82CB74A0 = (unsigned int)-1;
    s_numBlurPasses = 1;

    rdebugflags.volfogBlurPasses = 1;
    rdebugflags.volfogEnable = 0;
    rdebugflags.volfogEnableDebug = 0;

    //matInitialise();
    //engineInitialiseShaderUsers();
    /*inInitialiseHW();

    dword_82B1B74C = 0;
    lastKeyTime = 0.0;
    lastKeyRepeat = 0;*/

    int* streamHandle = &s_obStream[0].streamHandle;
    do
    {
        *(streamHandle - 1) = (unsigned int)-1;
        *streamHandle = (unsigned int)-1;
        *(streamHandle + 1) = 0;
        streamHandle += 3;
    } while ((int)streamHandle < (int)&obinstListDelete.offset);

    obInitialiseHW();
    //obInitialiseGfx();
    
    queueInit(&s_bgStreamQueue, 0x19, 0x108);
    bgReset();

    debugdrawInitialise(initData.DebugDrawBufferSize);
    fontInitialise(initData.FontBufferSize);

    if (s_menusTerminated)
    {
        currentH = (unsigned int)-1;
        MenuBlockType* p_Type = &menus[0].Type;
        do
        {
            *((unsigned int*)p_Type - 1) = 0;
            *p_Type = MENU_BLK_UNUSED;
            *((unsigned int*)p_Type + 7) = 0;
            *((unsigned int*)p_Type + 8) = 0;
            *((unsigned int*)p_Type + 9) = 0;
            p_Type += 73;
        } while ((int)p_Type < (int)&algn_82D85A98[4]);

        prevCur = 0;
        justClosed = 0;
        s_menusTerminated = 0;
    }

    sceneDescriptorInitialise();
    skyInitialise();
    cloudInitialise();

    unsigned int poolSize = (initData.Viewer == VIEWER_EDITOR) ? 2048 : 1024;
    
    //0xCu SceneObject?
    poolInitFromHeap(&objpool, 0, 0xCu, poolSize);

    scenelist.offset = 0;
    scenelist.head = (slinkdef_s*)&scenelist;

    //renderBatchReloadShaderInfo();

    s_flares = (flare_s*)memAllocAlignCore(1536, 0, 0, "source/renderer/flaregfx.c", 278, "", 1);
    s_sphere = (vec3_u*)memAllocAlignCore(384, 0, 0, "source/renderer/flaregfx.c", 279, "", 1);
    //flareGenerateSphere();
    memset(s_flares, 0, 0x600);
    s_flareNum = 0;

    s_explosions = (explosion_s*)memAllocAlignCore(512, 0, 0, "source/renderer/explosiongfx.c", 124, "", 1);
    memset(s_explosions, 0, 0x200);
    s_explosionNum = 0;

    //taskmanInitialise();
    //soundInitialise(initData.soundEnable);
    //constantsInitialise();
    //clothInitialise();

    //Fuck Physics
    //s_bonechainList.m_heap = 0;
    //listInitFromHeap(&s_bonechainList.m_list, 0, 0x80, 4);

    pakInitialise();

    s_glows = (unsigned int*)memAllocAlignCore(512, 0, 0, "source/renderer/glowgfx.c", 295, "", 1);
    s_drawGlows = (unsigned int*)memAllocAlignCore(64, 0, 0, "source/renderer/glowgfx.c", 296, "", 1);
    glowReset();

    s_defaultTexture = texLoadTextureName("misctex/particle/ion_star", 0);

    if (!rdebugflags.shaderJitCompile)
        g_streamShaders = 1;

    worldData.numobs = 256;
    worldData.numprops = 1500;
    engineframe = 0;
}

double timerGetImmediateTime()
{
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);

    __int64 elapsed = counter.QuadPart - timerGameStartTime.QuadPart;
    return (double)elapsed / (double)timerFrequency.QuadPart;
}

void timerSleep(float t)
{
    if (!_isnan(t) && _finite(t))
        Sleep((DWORD)(t * 1000.0f));
}

void lockFrameRate(double renderFrameStart)
{
    if (!rdebugflags.frameratelock)
        return;
    if (!rdebugflags.frameratelockEnabled)
        return;

    // Calculate target frame end time
    // frameratelock = target FPS, so frame duration = 1.0 / (frameratelock * 10.0)
    float frameDuration = 1.0f / ((float)rdebugflags.frameratelock * 10.0f);
    double targetTime = (double)frameDuration + renderFrameStart;

    // Sleep most of the remaining time minus a small margin
    double immediateTime = timerGetImmediateTime();
    double remaining = targetTime - immediateTime;
    if (remaining > 0.002000000094994903)
        timerSleep((float)(remaining - 0.002000000094994903));

    // Busy-wait spin for the last ~0.1ms for precision
    LARGE_INTEGER counter;
    double elapsed = 0;
    do
    {
        QueryPerformanceCounter(&counter);
        elapsed = (double)(counter.QuadPart - timerGameStartTime.QuadPart)
            / (double)timerFrequency.QuadPart;
    } while (targetTime - elapsed > 0.00009999999747378752);
}

unsigned __int8 s_paused = 0u;
double renderFrameStart = 0;
int g_numTltTest = 0;
unsigned __int16 s_frameId = 0;

int engineTickWorld()
{
    // --- One-time init ---
    static bool oneTime = true;
    if (oneTime)
    {
        if (!rdebugflags.shaderJitCompile)
            g_streamShaders = 1;
        oneTime = 0;
    }

    // --- Timing ---
    timerTick(s_paused);
    float dt = timerLastGameFrameDuration;   // f31, reused throughout

    //CXenonNotificationHandler::Tick(&s_notificationHandler, dt);
    //if (s_tltWaitFunc)
    //    s_tltWaitFunc();

    //// --- Memory context defrag ---
    //for (hmemContext* ctx = s_contexts; ctx; ctx = ctx->next)
    //    contextDefragTick(ctx);

    //// --- Memory test ---
    s_memAllocsThisFrame = 0;
    //pmemIntTest(memgroups->groupstart, memgroups->groupend);
    //if (!testOut)
    //{
    //    testHostName[0] = 0;
    //    testUserName[0] = 0;
    //    testIPString[0] = 0;
    //}

    //// --- I/O and task sync ---
    fileTick();
    //inTick();
    fileTick();
    //clothTaskWaitForAll();

    //// --- Physics end sync ---
    //if (!rdebugflags.noenginetick && engineWaitForPhysicsEnd)
    //    engineWaitForPhysicsEnd();

    //// --- World ticks ---
    //rterrainTick();
    //bgTick();

    //if (!rdebugflags.noenginetick && engineTickWorldFunc)
    //    engineTickWorldFunc();

    //obTick();
    //vmTick();

    //if (!s_paused)
    //    particleTick();

    //// --- Physics begin ---
    //if (!rdebugflags.noenginetick && enginePhysicsBeginTick)
    //    enginePhysicsBeginTick();

    //// --- Asset streaming ---
    //fileTick();
    //texTick();
    //fileTick();

    // --- Effects (game-time only) ---
    if (!s_paused)
    {
       /* sceneDescriptorTickTransition(dt);
        electricTick();
        flareTick();
        explosionTick();
        lightningTick();
        dustTick();
        spaceDustTick();*/
    }

    //// --- Environment ---
    fileTick();
    //if (s_bEnable_1)
    //{
    //    if (!s_bInitialised_0)
    //        cloudReset();
    //    cloudAreaTickArea(&s_cloudArea);
    //    cloudLayerTickLayer(&s_cloudLayer);
    //}
    //weatherTick();
    fileTick();
    //trackfxTick();
    //decalTick();
    fileTick();
    fileTick();
    //obinstTick();

    // --- TLT sync point 2 ---
    //if (s_tltWaitFunc)
    //    s_tltWaitFunc();

    // --- Prop / scene setup ---
    //generatePropList();
    g_numTltTest = 0;
    ++s_frameId;

    //// --- Bone chain tick ---
    //int count = s_bonechainList.m_list.count;
    //for (int i = 0; i < count; ++i)
    //    frdphysics::PhysicsBoneChain::Tick(s_bonechainList.m_list.items[i], dt);

    //// --- Cloth / rendering support ---
    //clothTick(dt);
    //lightTick();
    fileTick();
    //soundTick();
    fileTick();
    //rendererDebugTick();

    // --- Portal / view visibility ---
    int numActiveViews = 0;
    //opaqueView* activeViews[18];

    //for (slinklistdef_s* node = viewListAll.head;
    //    node != &viewListAll;
    //    node = node->head)
    //{
    //    opaqueView* view = (opaqueView*)((char*)node + viewListAll.offset);

    //    if (!(view->flags & 0x10))   // bit 4: "is active" or "is portal"
    //        continue;

    //    activeViews[numActiveViews++] = view;

    //    if (s_guPortals.isValid)
    //    {
    //        int rNum;
    //        int inst = bgFindInst(&view->pos, &rNum);
    //        if (inst != -1)
    //        {
    //            view->roomNum_prev = rNum;
    //            if (inst != view->inst)
    //            {
    //                // Room changed — update room group
    //                BgRoomGroup_s group;
    //                bginstGetRoomGroupFromRoomNum(inst, rNum, &group);
    //                view->inst = inst;
    //                view->roomGroup = group;
    //            }
    //        }
    //    }
    //}

    //sceneSetupLodsForSkinnedModels(numActiveViews, activeViews);

    //// --- Streaming / materials / foliage ---
    //fileStreamTick();
    //matTick();
    //foliageTick();

    // --- Frame rate lock + buffer swap ---
    lockFrameRate(renderFrameStart);   // waits until frame budget elapsed
    platformSwapBuffersHW();

    // --- Record render frame start time (after swap) ---
    {
        LARGE_INTEGER qpc;
        QueryPerformanceCounter(&qpc);
        int64_t elapsed = qpc.QuadPart - timerGameStartTime.QuadPart;
        renderFrameStart = (double)elapsed / (double)timerFrequency.QuadPart;
    }

    // --- Render ---
        //PC PORT
    if (WinWindows::GameWindow && WinWindows::GameWindow->msg.message != WM_QUIT)
    {
        WinWindows::GameWindow->Run();
        BeginScene();
        frameResetRenderState();
        camRenderAllViews();
        EndScene();
    }
    else
    {
        WorldState.WorldState = 2;
    }

     //--- Optional FPS cap spin-wait ---
    if (rdebugflags.frameratelock)
    {
        // Compute deadline: timerFrameStartTime + (1.0f / targetFPS)
        float targetFPS = (float)rdebugflags.frameratelock;
        double deadline = (double)(1.0f / targetFPS) + timerFrameStartTime;

        LARGE_INTEGER qpc;
        double now;
        do {
            QueryPerformanceCounter(&qpc);
            int64_t elapsed = qpc.QuadPart - timerGameStartTime.QuadPart;
            now = (double)elapsed / (double)timerFrequency.QuadPart;
        } while (now < deadline);
    }

    ++engineframe;

    return 1;
}

int s_Width = 1280;
int s_Height = 720;
unsigned __int8 s_usingCortez = 0u;
unsigned __int8 s_usingGUIEditor = 0u;
unsigned __int8 s_usingPhoto = 0u;
unsigned __int8 s_usingFacial = 0u;
unsigned __int8 s_usingAnimTool = 0u;
unsigned __int8 g_worldEditor = 0u;
bool s_vmOnly = false;

void checkCommandLineArguments(EngineInitData* data, int* io_argc, char*** io_argv)
{

}

bool s_quitflag = false;
void mainSetQuitFlag(unsigned int inRef)
{
    s_quitflag = 1;
}

void InitWorld()
{
    STUB_STATIC();
}

void DeInitWorld()
{
    STUB_STATIC();
}

void TickWorld()
{
    STUB_STATIC();
}

void PhysicsStartTick()
{
    STUB_STATIC();
}

void PhysicsWaitForEnd()
{
    STUB_STATIC();
}

float(__cdecl* s_gameTimeCalculate_0)(double);
float(__cdecl* s_systemTimeCalculate_0)(double);

// Static state (file-scope)
static float   s_deltas[4];
static int     s_currentDelta = 0;
static int     s_numDeltasRecorded = 0;
unsigned __int8 s_VidCap = 0u;

float s_systemTimeCalculate(double timeIn)
{
    if (g_constants->k_misc_filterTimeDelta)
    {
        // Write incoming delta into circular buffer
        s_deltas[s_currentDelta] = (float)timeIn;

        // Advance write cursor, wrap at 4
        if (++s_currentDelta >= 4)
            s_currentDelta = 0;

        // Track how many valid entries exist, cap at 4
        int count = s_numDeltasRecorded + 1;
        if (count > 4) count = 4;
        s_numDeltasRecorded = count;

        // Sum all valid entries (unrolled 4-wide in asm, logic is just a sum)
        float sum = 0.0f;
        for (int i = 0; i < count; ++i)
            sum += s_deltas[i];

        // Replace timeIn with the running average
        timeIn = sum / (float)count;
    }

    // Video capture mode: lock to exactly 60fps timestep
    if (s_VidCap)
        return 1.0f / 60.0f;  // 0x3c888889 == 0.016666668f

    return (float)timeIn;
}

float s_gameTimeCalculate(double timeIn)
{
    /*inControllerDef* controller = NULL;

    if (inGetControllerHW(0, &controller))
    {
        if (inputModeKeyHeld(2u, controller, 0x2Cu) &&
            (inputModeKeyHeld(2u, controller, 0x85u) || inputModeKeyHeld(2u, controller, 0x86u)))
        {
            timeIn = g_constants->k_slowDown_timesliceModifier * (float)timeIn;
        }

        if (inputModeKeyHeld(2u, controller, 0x2Eu) &&
            (inputModeKeyHeld(2u, controller, 0x85u) || inputModeKeyHeld(2u, controller, 0x86u)))
        {
            timeIn = g_constants->k_speedUp_timesliceModifier * (float)timeIn;
        }
    }

    if (g_constants->k_misc_clampTimeDelta)
    {
        float maxDelta = CMgrT<CPhysicsMgr>::s_mgr->getTimeClamp(CMgrT<CPhysicsMgr>::s_mgr);
        if ((float)timeIn > maxDelta)
            return maxDelta;
    }*/

    return (float)timeIn;
}

unsigned int timerTickFrameNum = 0u;

void timerTick(bool paused)
{
    ++timerTickFrameNum;

    // Guards against QPC glitches or uninitialized timerFrameStartTime
    double wallTime, frameDelta;
    do
    {
        LARGE_INTEGER qpc;
        QueryPerformanceCounter(&qpc);
        int64_t elapsed = qpc.QuadPart - timerGameStartTime.QuadPart;
        wallTime = (double)elapsed / (double)timerFrequency.QuadPart;
        frameDelta = wallTime - timerFrameStartTime;
    } while (frameDelta < 0.0);


    float sysDelta = (float)frameDelta;
    if (s_systemTimeCalculate)
        sysDelta = s_systemTimeCalculate(sysDelta);   // smoothing / VidCap override
    timerLastFrameDuration = sysDelta;

    if (paused)
    {
        timerLastGameFrameDuration = 0.0f;
        timerFrameStartTime = wallTime;
        return;
    }

    float gameDelta = sysDelta;
    if (s_gameTimeCalculate)
        gameDelta = s_gameTimeCalculate(gameDelta);   // slow-mo / speed-up modifier

    timerLastGameFrameDuration = gameDelta;
    ++timerGameFrameNum;

    double newGameTime = timerGameTime + gameDelta;
    timerGameTime = newGameTime;

    if (timerTargetGameTime <= 0.0)
    {
        timerFrameStartTime = wallTime;
        return;
    }

    double target = timerTargetGameTime + frameDelta;
    timerTargetGameTime = target;

    // How far are we from the target?
    double error = target - newGameTime;           // signed
    double absError = (target > newGameTime) ? error : -error;
    double nudge = gameDelta * error;              // proportional correction

    const double kSnapThreshold = 0.125;             // 0x3fc0000000000000 == 0.125

    if (absError > kSnapThreshold || nudge > kSnapThreshold)
    {
        // Error too large to smooth — snap directly to target
        timerGameTime = target;
    }
    else
    {
        // Small error — blend toward target
        timerGameTime = newGameTime + nudge;
    }

    timerFrameStartTime = wallTime;

    // Once game time has caught up to target, clear it
    if (timerGameTime >= target)
        timerTargetGameTime = 0.0;
}

void constantsInitFramework(const char* path, int inUserData)
{
    g_constants = new f_constants_res();
}

void constantsInitAI(const char* inFile, void* inUserData)
{
    g_constants_ai = new constants_ai_res();
}

void constantsInitVehicles(const char* inFile, void* inUserData)
{
    //Res file is empty
}

void constantsInitHealthDamage(const char* inFile, void* inUserData)
{
    g_constants_healt_hand_damage = new constants_health_and_damage_res();
}

void constantsInitBFConstantsAI(const char* inFile, void* inUserData)
{
    g_bf_constants_ai = new bf_constants_ai_res();
}

void constantsInitBfConstants(const char* inFile, void* inUserData)
{
    g_bf_constants = new bf_constants_res();
}
int camsUsed;
unsigned __int8 engineInWorld = 0;
int engineInitialiseWorld()
{
    engineInWorld = 1;
    viewListAll.offset = 0;
    viewListAll.head = (slinkdef_s*)&viewListAll;
    camsUsed = 0;

    timerGameFrameNum = 0;
    timerGameTime = 0.0;
    timerTargetGameTime = 0.0;

    return 1;
}

void constantsInitAll()
{
    char path[0x100];

    // Framework constants -- try project-specific first, fall back to common
    snprintf(path, 0x100, "data\\%s\\constants\\f_constants.res", g_project);
    {
        unsigned __int8 saved = s_fileAccessAllowed;
        s_fileAccessAllowed = 1;
        unsigned int size = fileSize(path);
        s_fileAccessAllowed = saved;
        if (size == (unsigned int)-1)
            strncpy(path, "data\\common\\constants\\f_constants.res", 0x100);
    }
    constantsInitFramework(path, 0);

    // AI constants -- try project-specific first, fall back to common
    snprintf(path, 0x100, "data\\%s\\constants\\constants_ai.res", g_project);
    {
        unsigned __int8 saved = s_fileAccessAllowed;
        s_fileAccessAllowed = 1;
        unsigned int size = fileSize(path);
        s_fileAccessAllowed = saved;
        if (size == (unsigned int)-1)
            strncpy(path, "data\\common\\constants\\constants_ai.res", 0x100);
    }
    constantsInitAI(path, 0);

    // Always use common for these
    constantsInitVehicles("data\\common\\constants\\constants_vehicles.res", 0);
    constantsInitHealthDamage("data\\common\\constants\\constants_health_and_damage.res", 0);

    // Project-specific constants based on g_project
    if (strcmp(g_project, "bf") == 0 || strcmp(g_project, "iv") == 0)
    {
        constantsInitBFConstantsAI("data\\bf\\constants\\bf_constants_ai.res", 0);
        constantsInitBfConstants("data\\bf\\constants\\bf_constants.res", 0);
    }
    //else if (strcmp(g_project, "ts") == 0)
    //{
    //    constantsInitBFConstantsAI("data\\bf\\constants\\bf_constants_ai.res", 0);
    //    constantsInitBfConstants("data\\bf\\constants\\bf_constants.res", 0);
    //    constantsInitTSConstants("data\\ts\\constants\\ts_constants.res", 0);
    //}
}

void frdmain(int argc, char** argv)
{
    DbgPrint("frdmain");
    WorldState.s_remakeAllMissingAssets = 1;

    // Zero the EngineInitData on the stack
    EngineInitData data = {};

    // Build version string constants
    // "FRDBVERSFRDSbranches/bf/ms_mar_08 70217..."
    // "branches/bf/ms_mar_08 70217 GOLD BUILD"
    const char* buildBranch = "branches/bf/ms_mar_08";
    const char* buildString = "FRDBVERSFRDSbranches/bf/ms_mar_08 70217...";
    const char* buildDisplay = "branches/bf/ms_mar_08 70217 GOLD BUILD";
    const char* buildNumber = "70217";
    DbgPrint("frdmain1");
    g_buildStuff = buildString;
    g_buildStuffForDisplay = buildDisplay;

    data.shadowQuality = s_shadowQuality;
    data.pixelAspect = 1.0f;   // k_footVelForImpactEffect
    data.soundEnable = s_soundEnable;

    // Parse build number string -> integer
    data.buildNumber = (DWORD)atol(buildNumber);

    // Format build info string into WorldState buffer

    StVafmtT<512> fmt1("build=%s %s", buildNumber, buildBranch);
    vafmtbuff_add(WorldState.s_buildInfo, 0x100, "%s  ", fmt1.str);
    DbgPrint("frdmain2");
    std::cout << fmt1.str << std::endl;
    bossInitialise();

    // Allocate 0xC00000 bytes of physical memory for the heap

    void* heap = malloc(0xC000000);  // 192MB - matches XPhysicalAlloc(0xC000000)

    //void* heap = VirtualAlloc((void*)0x02000000, 0xC000000,
    //    MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    memInitialise((unsigned char*)heap, 0xC000000);

    DbgPrint("frdmain3");
    // Parse "project=" from argv
    vafmtbuff(g_project, 0x20, "bf");  // default project = "bf"
    for (int i = 1; i < argc; i++)
    {
        if (strncmp(argv[i], "project=", 8) == 0)
        {
            vafmtbuff(g_project, 0x20, "%s", argv[i] + 8);
            break;
        }
    }
    DbgPrint("frdmain4");
    if (g_project[0] == '\0')
        exit(1);

    fileInitialise();

    StVafmtT<512> fmt2("project=%s", g_project);
    vafmtbuff_add(WorldState.s_buildInfo, 0x100, "%s  ", fmt2.str);

    checkCommandLineArgumentsEarly(argc, argv);

    // Detect project type from g_project string (strcmp against "g5")
    bool isG5 = (strcmp(g_project, "g5") == 0);

    memSettings_s* memSettings = isG5 ? &memorySettingsG5 : &memorySettingsBF;
    DbgPrint("frdmain5");
    // Scan argv for "-stage" and "-editor" flags
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-stage") == 0) { g_usingStage = 1; continue; }
        if (strcmp(argv[i], "-editor") == 0) { s_usingEditor = 1; continue; }
    }

    // Pick hash dict heap number based on tool mode
    int hashDictHeapNumber = (s_usingEditor || g_usingStage)
        ? memSettings->notInGameHashDictHeapNumber
        : memSettings->hashDictHeapNumber;

    dictionaryInitialise(
        memSettings->dictContentsHeapSize,
        hashDictHeapNumber,
        memSettings->packedDictNumber,
        memSettings->fixedSizeDictHeapSize,
        memSettings->fixedSizeDictHeapNumber,
        memSettings->fixedSizeArrayHeapSize,
        memSettings->fixedSizeArrayHeapNumber);
    DbgPrint("pool offset=%d", (int)offsetof(memgroupdef_s, ___u9.fixed.pool));
    memgroupdef_s* grp = &memgroups[CHashDictionary::s_objectHeap];
    DbgPrint("hashDict: objectSize=%d freeCount=%d free=%p objects=%p",
        grp->___u9.fixed.pool.objectSize,
        grp->___u9.fixed.pool.freeCount,
        grp->___u9.fixed.pool.free,
        grp->___u9.fixed.pool.objects);
    DbgPrint("s_objectHeap=%d s_heapDictContents=%d",
        CHashDictionary::s_objectHeap, CDictionary::s_heapDictContents);

    mainInitProjectSettings(argc, argv);
    checkCommandLineArguments(&data, &argc, &argv);

    // Magic value 0x186A0 = special debug argc sentinel — run cmdline args twice
    if (argc == 0x186A0)
        checkCommandLineArguments(&data, &argc, &argv);
    DbgPrint("frdmain6");
    // Set screen resolution
    if (s_usingEditor)
    {
        data.screenResX = 800;   // 0x320
        data.screenResY = 600;   // 0x258
    }
    else
    {
        data.screenResX = s_Width;
        data.screenResY = s_Height;
    }

    data.DebugDrawBufferSize = 0x18000;
    data.Viewer = ViewerType::VIEWER_QUICKVIEWER;

    // Determine viewer type — any non-game tool mode = editor viewer (2)

    bool isToolMode = s_usingEditor || s_usingCortez || s_usingGUIEditor ||
        s_usingAnimTool || s_usingPhoto || g_usingStage ||
        g_worldEditor || s_usingFacial;
    data.Viewer = (ViewerType)(isToolMode ? 2 : 1);

    DbgPrint("frdmain7");
    // Read lockframerate setting from g_settings dictionary

    TDictDataValue lockFramerate;
    //g_settings->GetOptionalIntValue("lockframerate", &lockFramerate, 0);
    lockFramerate.i = g_settings->lockFramerateTo;
    if (lockFramerate.i > 60) lockFramerate.i = 0;
    data.lockFramerateTo = lockFramerate.i;

     //VM-only mode — compile shaders and exit
    //Should never be enabled but we will implement later
    if (s_vmOnly)
    {
        //constantsInitialise();
        //constantsInitAll();

        //if (s_memBoost)
        //    memoryMake(0xA00000);
        //else
        //    memoryMake(0);

        //registerClasses();
        //dictionaryLoadObjectFromFile("data/common/mgrs", "mgrmgr", 0x6D67726D, nullptr, false, 0);
        //DbgPrint("frdmain8");
        //CMgrMgr* mgrMgr = CMgrMgr::s_singleton;

        //mgrMgr->m_restartCallback = mgrMgrRestartCallback;

        //mgrMgr->MgrLoad("preloadmgr.res", "preloadmanager", 0x706C6D72, true, true);

        //vmReset();
        //mgrMgr->MgrLoad("vmmgr.res", "vmmgr", 0x636D6772, true, true);
        //doVmBatchCompile();
        //DbgPrint("frdmain9");
        //exit(0);
    }

    // Normal game path
    WorldState.s_engineQuitFunc = (void*)mainSetQuitFlag;

    engineInitialise(&data);

    bool forceVSync = g_settings->forceVsync;
    //g_settings->GetOptionalBoolValue("forcevsync", &forceVSync, false);
    DbgPrint("frdmain10");

     //Store world lifecycle function pointers

    
    WorldState.engineInitialiseWorldFunc  = InitWorld;
    WorldState.engineDeinitialiseWorldFunc= DeInitWorld;
    WorldState.engineTickWorldFunc        = TickWorld;
    WorldState.enginePhysicsBeginTick     = PhysicsStartTick;
    WorldState.engineWaitForPhysicsEnd    = PhysicsWaitForEnd;

    // Point the time calculate function pointers
     //(s_gameTimeCalculate_0 / s_systemTimeCalculate_0 are the live slots)
    s_gameTimeCalculate_0 = s_gameTimeCalculate;
    s_systemTimeCalculate_0 = s_systemTimeCalculate;

    constantsInitAll();
    DbgPrint("frdmain11");
    // Main game loop — dispatch on WorldState
    for (;;)
    {
        switch (WorldState.WorldState)
        {
        case 0: // UNINITIALISED — trigger world init
            engineInitialiseWorld();
            DbgPrint("WorldState 0");
            WorldState.WorldState = 1;
            break;

        case 1: // INITIALISED — tick if in world, else request load
            if (engineInWorld)
            {
                engineTickWorld();
            }
            else
            {
                WorldState.WorldState = 2;
            }
            break;

        case 2: // RUNNING — deinit world
            //if (WorldState.engineDeinitialiseWorldFunc)
            //    WorldState.engineDeinitialiseWorldFunc();
            DbgPrint("WorldState 2");
            WorldState.WorldState = 0;
            return;

        case 3: // >= 3 — spin (loading / transitioning)
        default:
            DbgPrint("WorldState 3");
            break;
        }
    }
}
#define DedicatedServer

int main()
{
    DbgPrint("Main");

    //This function is almost 1 to 1 from XBOX. Common shit you know

    char* cmdline = (char*)GetCommandLineA();
    int cmdlen = (int)strlen(cmdline);

    int argc = 0;
    int pos = 0;
    char* argv[16];

    if (cmdline[0] != '\0')
    {
        do
        {
            if (pos >= cmdlen || argc >= 16)
                break;
            while (cmdline[pos] == ' ')
                cmdline[pos++] = '\0';
            argv[argc++] = &cmdline[pos];

            while (cmdline[pos] != ' ' && pos < cmdlen)
                pos++;
        } while (cmdline[pos] != '\0');
    }


    int processedArgc = engineHandleArguments(argc, argv);

    //Xbox stuff lets just ignore
    //SetUnhandledExceptionFilter(XenonExceptionHandler);

    //idk and idc atm
    //_set_purecall_handler((void(*)())CFrontendScene::TransparentOverlayGfx);

    initData.graphicsEnable = 1;
#ifdef DedicatedServer
    //Dedicated server
    FRDPeerHandler::s_peerHandler = new FRDPeerHandler();
    FRDSockets::m_instance = new FRDSockets();
    auto handle = new FRDNetPeer();
    handle->Initialize(0, 4, 60004, true, true, true, false);
    //Default Packet F0 0D 00 00 00 00 00
#endif // DEBUG
    frdmain(processedArgc, argv);

    return 0;
}


