#pragma once
#include <windows.h>

extern unsigned __int8 s_memInitDone;
extern unsigned int s_memMutex;

enum MemGroupType : __int32
{
    MEMGROUP_RANDOMSIZE = 0x0,
    MEMGROUP_FIXEDSIZE = 0x1,
    MEMGROUP_STACK = 0x2,
};


struct dlinkdef_s
{
    dlinkdef_s* next;
    dlinkdef_s* prev;
};

struct dlinklistdef_s
{
    dlinkdef_s* head;
    dlinkdef_s* tail;
    int offset;
};

struct $2E55D1C417D240E8327AC6E3B4FC6C0C
{
    dlinklistdef_s memblocks;
    dlinklistdef_s freeblocks;
};

union poolObject
{
    poolObject* next;
    unsigned __int8 user1[1];
    unsigned __int16 user2[1];
    unsigned int user3[1];
    unsigned __int64 user4[2];
    char user5[1];
    __int16 user6[1];
    int user7[1];
    __int64 user8[2];
    float user9[1];
    double user10[2];
    double user11[2];
};

struct poolState
{
    void* objects;
    poolObject* free;
    unsigned int freeCount;
    unsigned int totalCount;
    unsigned __int32 objectSize : 28;
    unsigned __int32 flags : 4;
};

struct $7BEEEBA8E9E4B4AD8E5169A42C9B28A5
{
    poolState pool;
};

struct $A4F24C89595507389B1399CA64475266
{
    dlinklistdef_s memblocks;
};

union $6D0DCA0B9789BB4B0993205583A29807
{
    $2E55D1C417D240E8327AC6E3B4FC6C0C random;
    $7BEEEBA8E9E4B4AD8E5169A42C9B28A5 fixed;
    $A4F24C89595507389B1399CA64475266 stack;
};

struct memgroupdef_s
{
    unsigned __int8 used;
    MemGroupType type;
    unsigned __int8 assertonfail;
    unsigned int flags;
    int freeBytes;
    char name[16];
    void* groupstart;
    void* groupend;
    unsigned int groupMutex;
    $6D0DCA0B9789BB4B0993205583A29807 ___u9;
};

extern memgroupdef_s memgroups[64];

struct MutexSlot
{
    DWORD  inUse;
    HANDLE handle;
};
#define MUTEX_HANDLE(idx)  (s_mutexSlots[idx].handle)

extern MutexSlot s_mutexSlots[16];

struct blockheaderdef_s
{
    unsigned __int8 used;
    unsigned __int8 flags;
    int size;
    dlinkdef_s freelink;
    dlinkdef_s link;
    unsigned int dataoffsetpadding[1];
};

struct memSettings_s
{
    int gameHeapSize;
    int physicsHeapSize;
    int eternalHeapSize;
    int dictContentsHeapSize;
    int hashDictHeapNumber;
    int notInGameHashDictHeapNumber;
    int packedDictNumber;
    int fixedSizeDictHeapSize;
    int fixedSizeDictHeapNumber;
    int fixedSizeArrayHeapSize;
    int fixedSizeArrayHeapNumber;
};

struct debugOverflow
{
    dlinkdef_s link;
    void* alloc;
    void* returnedAddress;
};

struct ConditionSlot
{
    DWORD  inUse;    // +0x00  0 = free, 1 = allocated
    HANDLE event;    // +0x04  CreateEventA handle
    DWORD  mutexId;  // +0x08  associated mutex index
};

struct stackheader_s
{
    unsigned int cur;
    unsigned int size;
    unsigned int datasize;
    void* data;
};

struct queueState_s
{
    unsigned __int16 count;
    unsigned __int16 maxSize;
    unsigned __int16 front;
    unsigned __int8* items;
};

struct poolObjArray_s
{
    void* array;
    poolObjArray_s* prev;
};

struct poolStateAE_s
{
    poolState state;
    int heap;
    poolObjArray_s* addObjArrays;
    unsigned int extendCount;
};

template<typename T>
struct CAutoExtendingPool
{
    poolStateAE_s m_poolae;
};

struct listState_s;

void* memAllocAlignCore(unsigned int length, unsigned int mgh, int align,
    const char* file, int line, const char* comment,
    char inFlags);

void groupLock(unsigned int mgh);
void groupUnlock(unsigned int mgh);
void* debugAlloc(int length, int inAlignment);
void memInitialise(unsigned __int8* buffer, unsigned int length);
void memFreeFlags(void* mem, unsigned __int8 inFlags);
memgroupdef_s* memGroupFindGroup(void* ptr);
void* memAllocAlignCore(unsigned int length, unsigned int mgh, int align,
    const char* file, int line, const char* comment,
    char inFlags);
void* memAllocAlignHighCore(int length, unsigned int mgh, int align,
    const char* file, int line, const char* comment,
    unsigned __int8 inFlags);
int memAllocGroupFixedBlockSize(
    const char* inName,
    int blockallocsize,
    unsigned int numblocks,
    unsigned int blockalign);
void poolAddObjectsArray(poolState* ioPool, char* inObjectsArray, unsigned int inObjectCount);
extern int g_heapDebug;
extern dlinklistdef_s s_heapDebugOverFlow;
extern __int8 mutexIsInitialised;
extern int g_heapTexStream;
extern int g_scratchStack;
extern BYTE g_assertsEnabled;
extern ConditionSlot s_conditionSlots[32];
extern memSettings_s memorySettingsG5;
extern memSettings_s memorySettingsBF;
extern int s_memAllocsThisFrame;
extern int g_heapGame;
extern _LARGE_INTEGER timerGameStartTime;
extern _LARGE_INTEGER timerFrequency;
void stackInit(stackheader_s* s, unsigned int numentries, unsigned int entrysize);
void queueInit(queueState_s* q, unsigned int itemCount, unsigned int itemSize);
void poolInitFromHeap(poolState* pool, unsigned int heap, unsigned int objectSize, unsigned int objectArrayLength);
void listInitFromHeap(listState_s* list, unsigned int heap, unsigned int itemCount, unsigned int itemSize);
unsigned int tsQueueCreate(unsigned int maxNumItems, unsigned int itemSize);
int memAllocGroupRandomBlockSizeEx(const char* inName, int groupmemsize, unsigned __int8 allocLow);
void* poolFreePool(poolState* pool);
void taskmanStartThreadExHW(HANDLE thread);
unsigned __int8 tsQueueAdd(unsigned int queueId, void* inItem,
    unsigned __int8 blocking);
void conditionWait(unsigned int conditionId);
double timerGetImmediateTime();