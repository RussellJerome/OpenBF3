#include "engstring.h"
#include "framework/template/CTemplate.h"
#include "engine/mem.h"
#include "../../Logger/Log.h"

char* vafmtbuff_add(char* outBuffer, int inSize, const char* format, ...)
{
    // find current string length
    int existingLen = (int)strlen(outBuffer);
    int remaining = inSize - existingLen;

    va_list ap;
    va_start(ap, format);
    _vsnprintf(outBuffer + existingLen, remaining, format, ap);
    va_end(ap);
    outBuffer[inSize - 1] = '\0';
    return outBuffer;
}

char* vafmtbuff(char* outBuffer, int inSize, const char* format, ...)
{
    if (inSize <= 0 || outBuffer == nullptr)
        return outBuffer;

    va_list ap;
    va_start(ap, format);
    _vsnprintf(outBuffer, inSize, format, ap);
    va_end(ap);
    outBuffer[inSize - 1] = '\0';
    return outBuffer;
}

#define EVA_SLOT_COUNT  5
#define EVA_SLOT_SIZE   1024    // 0x400 bytes, confirmed by li r4, 0x400 and slwi *1024

static char  s_evaString[EVA_SLOT_COUNT][EVA_SLOT_SIZE]; // .data:82ACB3C0
static DWORD s_evaRolling = 0;                            // .data:82CE10B4

char* evafmt(const char* format, ...)
{
    DWORD slot = (s_evaRolling + 1) % EVA_SLOT_COUNT;
    s_evaRolling = slot;

    char* dest = s_evaString[slot];
    va_list ap;
    va_start(ap, format);
    _vsnprintf(dest, EVA_SLOT_SIZE, format, ap);
    va_end(ap);
    dest[EVA_SLOT_SIZE - 1] = '\0';

    return dest;
}

int game_criticalAssert_fail(const char* expr, const char* inMsgPrefix,
    const char* inOrigMsg, const char* file,
    unsigned int line, const char* function)
{
    DbgPrint(inOrigMsg);
    return 0;
}

const char* CHashStrPoolBase::Copy(const char* inString)
{
    // Check if string already exists in pool
    void* existing = nullptr;
    if (stringTableFind(&m_stringTable, inString, &existing))
        return (const char*)existing;

    // Calculate string length including null terminator
    const char* p = inString;
    while (*p++) {}
    unsigned int len = (unsigned int)(p - inString);

    // Allocate a new element block if needed
    if (!m_eleHeadBlockFreeCount && !(m_flags & 1))
    {
        SHashStrPoolEleBlock* block = (SHashStrPoolEleBlock*)memAllocAlignCore(
            0x104u, m_contentsHeap, 4,
            "source/util/CHashStrPool.cpp", 247,
            "CHashStrPoolBase::AllocNewEleBlock", 1);
        if (block)
        {
            block->elements = (stringTableElement*)&block[1];
            block->next = m_eleBlockList;
            m_eleBlockList = block;
            m_eleHeadBlockFreeCount = 15;
        }
    }

    if (m_eleHeadBlockFreeCount <= 0)
        return (const char*)existing;

    // Pop a free element slot
    int freeIdx = m_eleHeadBlockFreeCount - 1;
    m_eleHeadBlockFreeCount = freeIdx;
    stringTableElement* ele = &m_eleBlockList->elements[freeIdx];

    if (!ele)
        return (const char*)existing;

    // Allocate string memory and copy
    char* dest = AllocStrMem((int)len);
    if (!dest)
        return nullptr;

    memcpy(dest, inString, len);
    stringTableAddWithStorageCore(&m_stringTable, ele, dest, dest);
    return dest;
}

char* CHashStrPoolBase::AllocStrMem(int inBytes)
{
    // If current block has enough space or expansion is disallowed
    if (inBytes > m_strBlockList->dataRemaining && !(m_flags & 2))
    {
        unsigned int allocSize;
        unsigned int mgh;

        if (inBytes <= 248)
        {
            allocSize = 260;  // 0x104
            mgh = m_contentsHeap;
        }
        else
        {
            allocSize = (unsigned int)(inBytes + 12);
            mgh = 0;
            if ((int)allocSize <= 0)
                goto done;
        }

        SHashStrPoolStrBlock* block = (SHashStrPoolStrBlock*)memAllocAlignCore(
            allocSize, mgh, 4,
            "source/util/CHashStrPool.cpp", 186,
            "CHashStrPoolBase::AllocStrMem", 1);

        if (block)
        {
            block->next = m_strBlockList;
            m_strBlockList = block;
            block->data = (char*)&block[1];
            block->dataRemaining = allocSize - 12;
        }
    }

done:
    SHashStrPoolStrBlock* cur = m_strBlockList;
    if (inBytes > cur->dataRemaining)
        return nullptr;

    char* result = cur->data;
    cur->dataRemaining -= inBytes;
    cur->data += inBytes;
    return result;
}
CHashStrPoolBase::CHashStrPoolBase(int inContentsHeap)
{
    m_contentsHeap = inContentsHeap;
    m_flags = 0;

    if (inContentsHeap == -1)
        m_contentsHeap = CDictionary::s_heapDictContents;

    m_flags = 0;
    m_stringTable.numEntries = 0;
    m_stringTable.length = 0;
    m_stringTable.index = nullptr;
    m_stringTable.mallocedStorage = NULL;
    m_eleBlockList = nullptr;
    m_strBlockList = nullptr;
    m_embeddedEleDataPtr = nullptr;
    m_embeddedEleDataSize = 0;
    m_embeddedStrDataPtr = nullptr;
    m_embeddedStrDataSize = 0;
}

void CHashStrPoolBase::InitStrPool(
    stringTableElement** inElePtrs,
    unsigned int          inNumElePtrs,
    char* inBaseEleBlockMemory,
    unsigned int          inBaseEleBlockMemorySize,
    char* inBaseStrBlockMemory,
    int                   inBaseStrBlockMemorySize,
    unsigned __int8       inFlags)
{
    m_flags = inFlags;
    m_stringTable.numEntries = 0;
    m_stringTable.length = inNumElePtrs;
    m_stringTable.index = inElePtrs;
    m_stringTable.mallocedStorage = NULL;

    if (inNumElePtrs)
        memset(inElePtrs, 0, 4 * inNumElePtrs);

    m_eleBlockList = nullptr;
    m_strBlockList = nullptr;

    if (inBaseEleBlockMemory)
    {
        m_embeddedEleDataPtr = inBaseEleBlockMemory;
        m_embeddedEleDataSize = inBaseEleBlockMemorySize;
        m_embeddedEleBlock.elements = (stringTableElement*)inBaseEleBlockMemory;
        m_embeddedEleBlock.next = m_eleBlockList;
        m_eleBlockList = &m_embeddedEleBlock;
        m_eleHeadBlockFreeCount = inBaseEleBlockMemorySize >> 4;
    }
    else
    {
        m_embeddedEleDataPtr = nullptr;
        m_embeddedEleDataSize = 0;
    }

    if (inBaseStrBlockMemory)
    {
        m_embeddedStrDataPtr = inBaseStrBlockMemory;
        m_embeddedStrDataSize = inBaseStrBlockMemorySize;
        m_embeddedStrBlock.next = m_strBlockList;
        m_strBlockList = &m_embeddedStrBlock;
        m_embeddedStrBlock.data = inBaseStrBlockMemory;
        m_embeddedStrBlock.dataRemaining = inBaseStrBlockMemorySize;
    }
    else
    {
        m_embeddedStrDataPtr = nullptr;
        m_embeddedStrDataSize = 0;
    }
}

unsigned int hashString(const char* string)
{
    if (!string)
        return 0;

    unsigned int h = 0;
    int c = (signed char)*string;

    while (c)
    {
        string++;
        h = (h + c) * 1024 + (h + c);  // h = (h+c) << 10 + (h+c) = 1025*(h+c)
        c = (signed char)*string;
        h = (h >> 6) ^ h;
    }

    h = h * 9;
    h = (h >> 11) ^ h;
    h = h * 32768 + h;  // h << 15 + h = 32769*h

    return h;
}

void stringTableAddWithStorageCore(stringTable* table, stringTableElement* buffer,
    const char* string, void* data)
{
    unsigned int hash = hashString(string);

    buffer->string = string;
    buffer->data = data;
    buffer->hashValue = hash;

    unsigned int bucketIdx = hash % table->length;
    stringTableElement** slot = &table->index[bucketIdx];

    buffer->next = *slot ? *slot : nullptr;
    *slot = buffer;

    table->numEntries++;
}

bool stringTableFind(const stringTable* table, const char* string, void** data)
{
    unsigned int hash = hashString(string);

    if (!table->length)
        return false;

    unsigned int bucket = hash % table->length;
    stringTableElement* entry = table->index[bucket];

    if (!entry)
        return false;

    while (entry)
    {
        if (entry->hashValue == hash)
        {
            // Compare strings
            const char* a = string;
            const char* b = entry->string;
            int diff;
            do
            {
                diff = (unsigned __int8)*a - (unsigned __int8)*b;
                if (!*a)
                    break;
                a++;
                b++;
            } while (!diff);

            if (!diff)
            {
                if (data)
                    *data = entry->data;
                return true;
            }
        }
        entry = entry->next;
    }

    return false;
}

const char* CStrPool::Copy(const char*)
{
    STUB();
    return nullptr;
}