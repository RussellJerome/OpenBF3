#include "CObjectAllocator.h"
#include "Engine/mem.h"
#include <cassert>

CObjectAllocator::CObjectAllocator(int inHeap, unsigned int inId, unsigned int seedNum, int flags)
{
    m_maxObjectSize = 0x2000;
    m_fallbackAllocator = nullptr;
    m_fixedSizeIntervalEnd = 0;
    m_fixedSizeIntervalAlign = 0;
    m_peakAlloc = 0;
    m_totalAlloc = 0;
    m_id = inId;

    memset(&m_state, 0, 0x30);

    m_state.heap = g_heapGame;
    m_state.sizes = (unsigned short*)memAllocAlignCore(
        0x802,
        g_heapGame,
        0,
        "source/mem/CObjectAllocator.cpp",
        263,
        nullptr,
        1);
    m_state.seedNum = 1000;
    m_flags = 0;
}

void CObjectAllocator::AddNewSize(unsigned short alignedSize, const char* inClassID)
{
    int sizesCount = m_state.sizesCount;

    // Check if size already registered
    bool found = false;
    for (int i = 0; i < sizesCount; i++)
    {
        if (m_state.sizes[i] == alignedSize)
        {
            found = true;
            break;
        }
    }

    if (!found && sizesCount < 1025)
    {
        m_state.sizes[sizesCount] = alignedSize;
        m_state.sizesCount = sizesCount + 1;
    }
}

void CObjectAllocator::RegisterClassNoKeyCopy(unsigned int inSize, unsigned int inProxySize, const char* inClassID)
{
    AddNewSize((unsigned short)((inSize + 15) & 0xFFF0), inClassID);

    if (inSize != inProxySize)
        AddNewSize((unsigned short)((inProxySize + 15) & 0xFFF0), inClassID);
}

void allocPhase(CObjectAllocator::TState* state, unsigned int inId, char flags)
{
    // Default seed num
    int seedNum = state->seedNum;
    if (!seedNum)
        seedNum = 100;

    // Find max size registered
    int sizesCount = state->sizesCount;
    unsigned short* sizes = state->sizes;
    int maxSize = 0;
    for (int i = 0; i < sizesCount; i++)
    {
        if (sizes[i] > maxSize)
            maxSize = sizes[i];
    }

    int tableSize = maxSize / 16 + 1;
    int doubleSize = maxSize * 2;

    // Calculate pool block size
    unsigned int poolSize;
    if (flags & 1)
    {
        // Round up to next power of 2 minus 1, align down to 16
        unsigned int v = doubleSize - 1;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        poolSize = (v + 48) & ~0xF;
    }
    else
    {
        // Round up to next power of 2
        unsigned int v = doubleSize + 31;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        poolSize = v + 1;
    }

    // Calculate pool count
    int poolCount;
    if (flags & 8)
    {
        // Scale seed num by pool size ratio
        poolCount = (int)((float)((float)state->seedNum / (float)poolSize) + 0.0099999998f);
    }
    else
    {
        poolCount = seedNum / 8;
    }
    if (poolCount <= 1)
        poolCount = 1;

    state->poolCount = poolCount;
    state->poolSize = poolSize;
    state->tableSize = tableSize;

    // Allocate main memory block
    int allocSize = 2 * (4 * sizesCount + tableSize) + (poolCount + 1) * poolSize + 19;

    void* mem = nullptr;
    if (allocSize > 0)
    {
        mem = memAllocAlignCore(
            allocSize, state->heap, 0,
            "source/mem/CObjectAllocator.cpp", 66,
            nullptr, 1);
    }

    state->freePtr = mem;
    state->endPtr = (char*)mem + allocSize;

    assert(poolSize > 0);

    // Align base pointer to poolSize boundary
    char* base = (char*)mem;
    unsigned int rem = (unsigned int)base % poolSize;
    if (rem)
        base = base - rem + poolSize;

    // Layout within allocation:
    // [poolCount * poolSize]  = pool blocks
    // [poolState header]      = pool-of-pools
    // [sizesCount * 8]        = dlinkdef_s array (pools)
    // [tableSize * 2]         = table (unsigned short array)

    char* poolBlocks = base;
    char* poolOfPoolsMem = base + poolCount * poolSize;
    dlinkdef_s* poolsArr = (dlinkdef_s*)(poolOfPoolsMem + 20);
    unsigned short* table = (unsigned short*)((char*)poolsArr + sizesCount * 8);

    state->poolOfPools = (poolState*)poolOfPoolsMem;
    state->pools = poolsArr;
    state->table = table;

    // Zero pools and table
    memset(poolsArr, 0, sizesCount * 8);
    memset(table, 0, tableSize * 2);

    // Initialize pool-of-pools
    poolState* pop = state->poolOfPools;
    pop->objects = poolBlocks;
    pop->free = nullptr;
    pop->freeCount = 0;
    pop->totalCount = 0;
    pop->objectSize = poolSize & 0xFFFFFFF;
    poolAddObjectsArray(pop, poolBlocks, poolCount);

    // Initialize each pool bucket as empty circular doubly-linked list
    for (int i = 0; i < sizesCount; i++)
    {
        dlinkdef_s* node = &poolsArr[i];
        node->next = node;
        node->prev = node;
    }

    // Build lookup table mapping size index -> pool bucket index
    int bucketIdx = 0;
    for (int sizeIdx = 0; sizeIdx < tableSize; sizeIdx++)
    {
        table[sizeIdx] = 0xFFFF;
        for (int i = 0; i < sizesCount; i++)
        {
            if (sizeIdx * 16 == sizes[i])
            {
                table[sizeIdx] = (unsigned short)bucketIdx++;
            }
        }
    }

    // Zero refCount field in each pool block
    char* block = poolBlocks + 30; // +0x1E
    for (int i = 0; i < poolCount; i++)
    {
        *(short*)block = 0;
        block += poolSize;
    }
}

void* CObjectAllocator::AllocObject(int inSize, const char* inClassID)
{
    TState* state = &m_state;

    if (!state->table)
        allocPhase(state, m_id, m_flags);

    if (m_flags & 4)
    {
        inSize = (m_fixedSizeIntervalAlign + inSize - 1) & ~(m_fixedSizeIntervalAlign - 1);
        if (inSize > m_fixedSizeIntervalEnd)
            return nullptr;
    }

    int alignedSize = (inSize + 15) & ~0xF;
    int sizeIndex = alignedSize / 16;

    unsigned short tableEntry = state->table[sizeIndex];
    dlinkdef_s* bucket = (dlinkdef_s*)((char*)state->pools + (tableEntry * 8));

    poolState* best = nullptr;

    if (bucket->next != bucket)
    {
        // Walk list to find pool with fewest free slots
        unsigned int bestCount = (unsigned int)-1;
        dlinkdef_s* node = bucket->next;
        do
        {
            // block is at link - 0x14
            poolState* block = (poolState*)((char*)node - 0x14);
            if ((unsigned int)block->freeCount < bestCount)
            {
                best = block;
                bestCount = (unsigned int)block->freeCount;
            }
            node = node->next;
        } while (node != bucket);
    }

    bool newBlock = false;

    if (!best)
    {
        newBlock = true;

        poolState* pop = state->poolOfPools;
        int        poolSize = state->poolSize;

        if (pop && pop->free)
        {
            best = (poolState*)pop->free;
            pop->free = *(poolObject**)pop->free;
            pop->freeCount--;
            goto got_block;
        }

        // Search larger size buckets
        for (unsigned int idx = sizeIndex; idx < state->tableSize; idx++)
        {
            unsigned short entry = state->table[idx];
            if (entry == 0xFFFF)
                continue;

            dlinkdef_s* poolNode = (dlinkdef_s*)((char*)state->pools + (entry * 8));
            if (poolNode->next != poolNode)
            {
                best = (poolState*)((char*)poolNode->next - 0x14);
                newBlock = false;
                goto got_block;
            }
        }

        // Allocate new pool block
        if (m_flags & 1 || state->poolSize <= 0)
            return nullptr;

        void* mem = memAllocAlignCore(
            state->poolSize, state->heap, state->poolSize,
            "source/mem/CObjectAllocator.cpp", 690,
            nullptr, 1);

        if (!mem)
        {
            mem = memAllocAlignCore(
                state->poolSize, 0, state->poolSize,
                "source/mem/CObjectAllocator.cpp", 730,
                nullptr, 1);
        }

        if (!mem)
            return nullptr;

        best = (poolState*)mem;
    }

got_block:
    if (newBlock)
    {
        assert(alignedSize > 0);

        best->objects = (char*)best + 0x20;
        best->free = nullptr;
        best->freeCount = 0;
        best->objectSize = (unsigned int)alignedSize;

        // refCount at +0x1E
        *(short*)((char*)best + 0x1E) = 1;

        int objCount = (state->poolSize - 0x20) / alignedSize;
        poolAddObjectsArray(best, (char*)best->objects, objCount);

        // Insert into bucket list
        unsigned short newEntry = state->table[sizeIndex];
        dlinkdef_s* newBucket = (dlinkdef_s*)((char*)state->pools + (newEntry * 8));
        dlinkdef_s* link = (dlinkdef_s*)((char*)best + 0x14);

        link->next = newBucket->next;
        link->prev = newBucket;
        newBucket->next = link;
        link->next->prev = link;
    }

    // Pop object from pool free list
    void* obj = best->free;
    if (obj)
    {
        best->free = *(poolObject**)obj;
        best->freeCount = best->freeCount - 1;
    }

    // If pool exhausted, unlink from bucket list
    if (best->freeCount == 0)
    {
        dlinkdef_s* link = (dlinkdef_s*)((char*)best + 0x14);
        link->prev->next = link->next;
        link->next->prev = link->prev;
        link->next = link;
        link->prev = link;
    }

    return obj;
}

void CObjectAllocator::FreeObject(void* inPtr)
{
    TState* state = &m_state;
    int     poolSize = state->poolSize;

    assert(poolSize > 0);

    // Find which pool block this pointer belongs to
    unsigned int addr = (unsigned int)inPtr;
    unsigned int blockAddr = addr - (addr % poolSize);
    poolState* block = (poolState*)blockAddr;

    int wasEmpty = (block->freeCount == 0);

    // Update total alloc tracking
    m_totalAlloc -= (block->objectSize & 0xFFFFFFF);

    // Push object onto pool free list
    *(void**)inPtr = block->free;
    block->free = (poolObject*)inPtr;
    block->freeCount++;

    dlinkdef_s* link = (dlinkdef_s*)((char*)block + 0x14);
    short* refCount = (short*)((char*)block + 0x1E);

    if (wasEmpty)
    {
        // Block was exhausted (not in bucket list), re-insert it
        int            sizeIdx = ((block->objectSize & 0xFFFFFFF) + 15) / 16;
        unsigned short tableEntry = state->table[sizeIdx];
        dlinkdef_s* bucket = (dlinkdef_s*)((char*)state->pools + (tableEntry * 8));

        link->next = bucket->next;
        link->prev = bucket;
        bucket->next = link;
        link->next->prev = link;
    }
    else if (block->freeCount == block->totalCount)
    {
        // Block is completely free -- unlink from bucket list
        link->prev->next = link->next;
        link->next->prev = link->prev;
        link->next = link;
        link->prev = link;

        poolFreePool(block);

        // Check if block is within the pool-of-pools managed range
        int offset = (char*)block - (char*)state->poolOfPools->objects;
        if (offset >= 0 && offset < (int)(state->poolCount * state->poolSize))
        {
            // Return block to pool-of-pools free list
            poolState* pop = state->poolOfPools;
            block->objects = pop->free;
            pop->free = (poolObject*)block;
            pop->freeCount++;
            *refCount = 0;
        }
        else
        {
            // Block was heap allocated, free it directly
            memFreeFlags((char*)block, 1);
            *refCount = 0;
        }
    }
}

CObjectAllocator::~CObjectAllocator()
{
    if (m_state.table)
        memFreeFlags((char*)m_state.freePtr, 1);

    if (m_state.sizes)
        memFreeFlags((char*)m_state.sizes, 1);
}

void CObjectFactory::RegisterClassWithProxyNoKeyCopy(
    const char* inClassID,
    unsigned int     objectTypeID,
    CBaseObject* (*inCreatorFunc)(const char*),
    unsigned int     inObjectSize,
    const char* inProxyClassID,
    unsigned int     proxyObjectTypeID,
    CBaseObject* (*inProxyCreatorFunc)(const char*),
    unsigned int     inProxyObjectSize)
{
    // Check if class already registered
    void* existing = nullptr;
    stringTableFind(&m_Hash.m_StringTable, inClassID, &existing);
    if (existing)
        return;

    // Check capacity
    unsigned int idx = m_Hash.m_UsedEntries;
    if (idx >= 0x401)
        return;

    m_Hash.m_UsedEntries = idx + 1;

    SObjectCreator* entry = &m_Hash.m_Entries[idx];

    stringTableAddWithStorageCore(
        &m_Hash.m_StringTable,
        &entry->ele,
        inClassID,
        entry);

    if (!entry)
        return;

    entry->creatorFunc = inCreatorFunc;
    entry->proxyCreatorFunc = inProxyCreatorFunc;
    entry->objectTypeID = objectTypeID;

    m_allocator->AddNewSize((unsigned short)((inObjectSize + 15) & 0xFFF0), inClassID);

    if (inObjectSize != inProxyObjectSize)
        m_allocator->AddNewSize((unsigned short)((inProxyObjectSize + 15) & 0xFFF0), inClassID);
}

CBaseObject* CObjectFactory::CreateObject(const char* inClassID, bool useProxy)
{
    SObjectCreator* entry = nullptr;
    stringTableFind(&m_Hash.m_StringTable, inClassID, (void**)&entry);

    if (!entry)
        return nullptr;

    if (useProxy)
        return entry->proxyCreatorFunc(inClassID);
    else
        return entry->creatorFunc(inClassID);
}

CObjectFactory* CObjectFactory::s_Factory = nullptr;
CObjectAllocator* s_allocator = nullptr;