#pragma once
#include <iostream>
#include <windows.h>

template<int N>
struct StVafmtT
{
    char str[N];

    StVafmtT(const char* format, ...)
    {
        va_list ap;
        va_start(ap, format);
        _vsnprintf(str, N, format, ap);
        va_end(ap);
        str[N - 1] = '\0';
    }
};

char* vafmtbuff_add(char* outBuffer, int inSize, const char* format, ...);
char* vafmtbuff(char* outBuffer, int inSize, const char* format, ...);
char* evafmt(const char* format, ...);

int game_criticalAssert_fail(const char* expr, const char* inMsgPrefix,
    const char* inOrigMsg, const char* file,
    unsigned int line, const char* function);

struct $348753DBBD1ED3412D232D24838D6FEA
{
    unsigned int lower;
    unsigned int upper;
};

union char8_s
{
    char str[8];
    $348753DBBD1ED3412D232D24838D6FEA __s1;
    unsigned __int64 whole;
    unsigned __int64 whole8cc;
};

union char16_s
{
    char chars[16];
    unsigned int u32s[4];
    unsigned __int64 u64s[2];
};

template<typename T>
struct CCharT
{
    T m_cc;
};

class CStrPool 
{
public:
    virtual ~CStrPool() {}
    virtual const char* Copy(const char* inString);
    virtual void Clear() {}
    virtual bool GetStrHash(const char*, uint32_t*) { return false; }
    virtual const char* GetStrFromHash(uint32_t) { return nullptr; }
};

struct stringTableElement
{
    const char* string;
    void* data;
    stringTableElement* next;
    unsigned int hashValue;
};

struct __declspec(align(4)) stringTable
{
    stringTableElement** index;
    unsigned int numEntries;
    unsigned int length;
    unsigned __int8 mallocedStorage;
};

template<typename T, int N>
class CFixedSizeStringTableT
{
public:
    virtual ~CFixedSizeStringTableT() {}

    stringTable         m_StringTable;
    stringTableElement* m_ElementPtrs[N];
    T                   m_Entries[N];
    unsigned int        m_UsedEntries;
};

struct SHashStrPoolEleBlock
{
    SHashStrPoolEleBlock* next;
    stringTableElement* elements;
};

struct SHashStrPoolStrBlock
{
    SHashStrPoolStrBlock* next;
    char* data;
    int dataRemaining;
};

class CHashStrPoolBase : public CStrPool
{
public:
    virtual ~CHashStrPoolBase() {}
    virtual const char* Copy(const char* inString) override;
    virtual void Clear() {}
    virtual bool GetStrHash(const char*, uint32_t*) override { return false; }
    virtual const char* GetStrFromHash(uint32_t) override { return nullptr; }

    char* AllocStrMem(int32_t);

    CHashStrPoolBase(int inContentsHeap);

    void InitStrPool(
        stringTableElement** inElePtrs,
        unsigned int          inNumElePtrs,
        char* inBaseEleBlockMemory,
        unsigned int          inBaseEleBlockMemorySize,
        char* inBaseStrBlockMemory,
        int                   inBaseStrBlockMemorySize,
        unsigned __int8       inFlags);

    unsigned __int8 m_flags;
    int m_contentsHeap;
    stringTable m_stringTable;
    SHashStrPoolEleBlock* m_eleBlockList;
    int m_eleHeadBlockFreeCount;
    SHashStrPoolStrBlock* m_strBlockList;
    SHashStrPoolEleBlock m_embeddedEleBlock;
    SHashStrPoolStrBlock m_embeddedStrBlock;
    char* m_embeddedEleDataPtr;
    char* m_embeddedStrDataPtr;
    int m_embeddedStrDataSize;
    int m_embeddedEleDataSize;
};

class CHashStrPool : public CHashStrPoolBase
{
public:
    CHashStrPool(int inContentsHeap)
        : CHashStrPoolBase(inContentsHeap)
    {
        InitStrPool(
            m_elePtrs, 0x100,
            (char*)m_embeddedEleData, 0x200,
            m_embeddedStrData, 0x400,
            0);
    }

    stringTableElement* m_elePtrs[256];
    void* m_embeddedEleData[128];
    char m_embeddedStrData[1024];
};

class CEmbeddedStrPool : public CStrPool
{
public:
    char* m_buffer;
    int m_bufferSize;
    char* m_nextStringPos;
    stringTable* m_stringTable;
    CEmbeddedStrPool* m_templateStrPool;
};

class __declspec(align(4)) CSimpleHashStrPool : public CHashStrPoolBase
{
public:
    stringTableElement* m_elePtrs[256];
    void* m_embeddedEleData[128];
    char* m_buffer;
    int m_bufferSize;
    bool m_inited;
};

bool stringTableFind(const stringTable* table, const char* string, void** data);
void stringTableAddWithStorageCore(stringTable* table, stringTableElement* buffer,
    const char* string, void* data);
