#include "CTemplate.h"
#include "CTemplateMgr.h"
#include "dictionary/CEmbeddedResFile.h"
#include "Logger/Log.h"
#include "engine/engineinit.h"
#include "file/packfile.h"



int CDictionary::s_heapDictContents = -1;
int CPackedStructure::s_objectHeap = -1;
IContainer::~IContainer()
{
    STUB();
}

int IContainer::CountEntries() const
{
    STUB();
	return 0;
}

bool IContainer::GetNthEntry(int inIndex, const char** outName, unsigned char* outType, TDictDataValue* outValue, __int16* outFlags)
{
    STUB();
	return false;
}

unsigned char IContainer::GetContainerType() const
{
    STUB();
	return 0;
}

int IContainer::GetHandle()
{
    STUB();
	return 0;
}

CTemplate* CEmbeddedDictionary::GetTemplate()
{
    //I assume this is it but idfk
    return nullptr;
}

CEmbeddedDictionary::CEmbeddedDictionary(CEmbeddedResFile* inResFile)
{
    m_resFile = (__int16)(((char*)inResFile - (char*)s_embeddedResFilePool.objects) / sizeof(CEmbeddedResFile));

    UTemplatePtrIdx current;
    current.idx = m_template.idx;

    if (current.idx == -1)
    {
        m_template.idx = 0;
    }
    else
    {
        m_template.ptr = (CTemplate*)&CTemplateMgr::s_mgr->m_hash.m_Entries[current.idx];
    }
}

const char* CTemplate::GetMetaDataFieldName()
{
    STUB();
    return nullptr;
}

bool CTemplate::GetDefaultValue(const char* inKey, unsigned __int8* outType,
    TDictDataValue* outValue, unsigned __int16* outFlags)
{
    // Build lookup key: "<templateName>.<inKey>"
    char lookupKey[256];
    unsigned int len = 0;

    // Copy template name
    const char* name = ele.string;
    while (*name && len < 0xFE)
        lookupKey[len++] = *name++;

    // Append '.'
    lookupKey[len++] = '.';

    // Append inKey
    const char* k = inKey;
    while (*k && len < 0xFF)
        lookupKey[len++] = *k++;

    lookupKey[len] = 0;

    // Look up in template manager's string table
    if (lookupKey[0])
    {
        void* entry = NULL;
        stringTableFind(&CTemplateMgr::s_mgr->m_hash.m_StringTable, lookupKey, &entry);
        if (entry)
        {
            if (outType)
                *outType = 8;
            if (outValue)
                outValue->d = (CDictionary*)((char*)entry + 8);
            if (outFlags)
                *outFlags = 0;
            return true;
        }
    }

    // Fall back to template's dict
    if (m_dict)
        return m_dict->GetOptionalValue(inKey, outType, outValue, outFlags);

    return false;
}

CTemplate::CTemplate()
{
    m_dict = nullptr;
    m_self.m_template = nullptr;
    m_isFieldMeta = false;

    // m_self sets its template pointer back to this CTemplate
    //m_self.SetTemplate(this);
}

void CTemplate::Init(CDictionary* inDict)
{
    m_dict = inDict;

    CTemplate* tmpl = inDict->GetTemplate();
    m_isFieldMeta = (tmpl && tmpl->m_isFieldMeta);
}

void CDictionary::SetTemplate(CTemplate*)
{
    STUB();
}

CTemplate* CDictionary::GetTemplate()
{
    STUB();
    return nullptr;
}

int CDictionary::CountValues() const
{
    STUB();
    return 0;
}

bool CDictionary::GetNthValue(int, const char**, unsigned char*, TDictDataValue*, __int16*)
{
    STUB();
    return false;
}

bool CDictionary::GetOptionalValue(const char* inKey, unsigned __int8* outType,
    TDictDataValue* outValue, unsigned __int16* outFlags)
{
    bool found = false;

    if (GetOptionalValueSelf(inKey, outType, outValue, outFlags))
    {
        found = true;
    }
    else
    {
        CTemplate* tmpl = GetTemplate();
        if (tmpl)
            found = tmpl->GetDefaultValue(inKey, outType, outValue, outFlags);
    }

    if (!found)
    {
        if (outValue)
            memset(outValue, 0, 4);
        if (outType)
            *outType = 0;
    }

    return found;
}

bool CDictionary::GetOptionalBoolValue(const char* inKeyName, bool* outValue, bool defaultValue)
{
    *outValue = defaultValue;

    unsigned __int8 type = 0;
    TDictDataValue  value = {};

    if (!GetOptionalValue(inKeyName, &type, &value, NULL))
        return false;

    if (type == 1)
    {
        *outValue = (value.i != 0);
        return true;
    }
    if (type == 4)
    {
        *outValue = (_stricmp(value.s, "TRUE") == 0);
        return true;
    }

    return false;
}

bool CDictionary::GetOptionalIntValue(const char* inKeyName, TDictDataValue* outValue, const int defaultValue)
{
    int result; // r3
    unsigned __int8 v6[4]; // [sp+50h] [-20h] BYREF
    outValue->i = defaultValue;
    if (CDictionary::GetOptionalValue(inKeyName, v6, outValue, 0))
    {
        if (v6[0] == 1)
        {
            result = 1;
            outValue->i = outValue->i;
            return result;
        }
        if (v6[0] == 2)
        {
            result = 1;
            outValue->i = (int)outValue->f;
            return result;
        }
    }
    return 0;
}

bool CDictionary::GetStringValue(const char* inKeyName, const char** outValue)
{
    bool found = GetOptionalStringValue(inKeyName, outValue);
    if (!found)
    {
        // Log warning — key not found in dict
        // Original calls GetTemplate() and GetDictSourceLocation() 
        // just to print a warning with source location info
        CTemplate* tmpl = GetTemplate();
        const char* file = nullptr;
        int line = 0;
        GetDictSourceLocation(&file, &line);
        DbgPrint("GetStringValue: key '%s' not found (source: %s:%d)",
            inKeyName, file ? file : "?", line);
    }
    return found;
}

bool CDictionary::GetOptionalStringValue(const char* inKeyName, const char** outValue)
{
    *outValue = NULL;

    unsigned __int8 type = 0;
    TDictDataValue  value = {};

    if (!GetOptionalValue(inKeyName, &type, &value, NULL))
        return false;

    if (type != 4)
        return false;

    *outValue = value.s;
    return true;
}

bool CDictionary::GetDictValue(const char* inKeyName, CDictionary** outDict)
{
    STUB();
    return false;
    /*bool result = GetOptionalDictValue(inKeyName, outDict);
    if (!result)
    {
        const char* file = NULL;
        int         line = 0;
        GetTemplate();
        GetDictSourceLocation(&file, &line);
    }
    return result;*/
}

bool CDictionary::GetOptionalValueSelf(const char* inKeyName, unsigned char* outType,
    TDictDataValue* outValue, unsigned short* outFlags)
{
    STUB();
    return false;
}

void CDictionary::EnterValue(
    const char* inHashKey, unsigned char inType,
    const TDictDataValue* inValue, unsigned short inFlags)
{
    STUB();
}

void CDictionary::EnterValueWithSourceLocation(const char* inKey, unsigned char inFlags,
    const TDictDataValue* inValue, unsigned short inKeyLen,
    const char* inSourceFileName, int inSourceLineNo)
{
    STUB();
}

bool CDictionary::GetDictSourceLocation(const char** outSourceFileName, int* outSourceLineNo)
{
    if (outSourceFileName)
        *outSourceFileName = nullptr;
    if (outSourceLineNo)
        *outSourceLineNo = -1;
    return false;
}

void CDictionary::SetDictSourceLocation(const char* inSourceFileName, int inSourceLineNo)
{
    STUB();
}

void CArray::GetArrayType(unsigned char*, CTemplate**) const
{
    STUB();
}

int32_t CArray::CountItems() const
{
    STUB();
    return 0;
}

bool CArray::GetNthItem(int32_t, TDictDataValue*, uint16_t*)
{
    STUB();
    return false;
}

bool CArray::AppendItem(TDictDataValue*, uint16_t)
{
    STUB();
    return false;
}

CEmbeddedArray::CEmbeddedArray(CEmbeddedResFile* inResFile)
{
    //STUB();
    //Must Define
}

CDictionaryFileParser::CDictionaryFileParser(CStrPool* inKeyPool, CStrPool* inStrValuePool,
    unsigned int inFlags, CList<CTemplate*>* templateList)
{
    m_KeyPool = inKeyPool;
    m_StrValuePool = inStrValuePool;
    m_flags = inFlags;
    m_rootDictSourceLineSet = false;
    m_templateList = nullptr;
    m_includeList = nullptr;
    m_includeListStrs = nullptr;
    m_StackIndex = 0;
    m_fileStackIndex = 0;

    // Zero first 3 stack entries
    m_Stack[0] = {};
    m_Stack[1] = {};
    m_Stack[2] = {};

    // Zero the file path prefix (4 bytes)
    memset(m_filePathPrefix, 0, 4);

    m_origFile[0] = '\0';
    m_allocator = nullptr;
}

char* CDictionaryFileParser::ParseFileName(char* outBuffer, int inBufferSize, const char* inFileNameConst)
{
    DbgPrint("ParseFileName");
    strncpy(outBuffer, inFileNameConst, 0x100);
    outBuffer[255] = 0;

    if (!g_settings)
        return outBuffer;

    // Handle $variable$ substitution
    if (strstr(inFileNameConst, "$"))
    {
        char bufA[257];
        char bufB[257];
        strncpy(bufA, inFileNameConst, 0x100);
        bufA[256] = 0;

        bool toggle = false;
        do
        {
            char* src = toggle ? bufB : bufA;
            char* dst = toggle ? bufA : bufB;
            char* out = dst;
            int   len = 0;

            while (*src)
            {
                if (*src == '$')
                {
                    src++;
                    char varName[256];
                    char* v = varName;
                    while (*src && *src != '$' && *src != '/')
                        *v++ = *src++;
                    *v = 0;

                    //CDictionary* pathDict = NULL;
                    //g_settings->GetDictValue("path", &pathDict);

                    bool vaildDict = false;
                    const char* value = NULL;
                    if (varName == "COMMON")
                    {
                        value = g_settings->Path.COMMON;
                    }
                    else if (varName == "VMDOC")
                    {
                        value = g_settings->Path.VMDOC;
                    }
                    //if (!(unsigned __int8)pathDict->GetOptionalStringValue(varName, &value))
                    //    return outBuffer;
                    if (!value)
                        return outBuffer;

                    int vlen = (int)strlen(value);
                    if (vlen + len >= 256)
                        return outBuffer;

                    char* d = &out[-vlen];
                    const char* s = value;
                    do { d[s - value] = *s; } while (*s++);
                    out += vlen;
                    len += vlen;
                }
                else
                {
                    if (len >= 256)
                        return outBuffer;
                    *out++ = *src++;
                    len++;
                }
            }
            *out = 0;
            toggle = !toggle;
        } while (strstr(toggle ? bufB : bufA, "$"));

        strncpy(outBuffer, toggle ? bufB : bufA, 0x100);
        outBuffer[255] = 0;
    }

    // Handle ".." path collapsing
    if (strstr(outBuffer, ".."))
    {
        char tmp[257];
        strncpy(tmp, outBuffer, 0x100);
        tmp[256] = 0;

        char* found = strstr(tmp, "..");
        while (found && found > tmp)
        {
            char* after = found + 2;
            char* p = found;
            bool  seenSlash = false;
            bool  seenChar = false;

            do
            {
                char c = *p;
                if (c == '\\' || c == '/')
                {
                    seenSlash = true;
                    if (seenChar) break;
                }
                else if (seenSlash)
                {
                    seenChar = true;
                }
                p--;
            } while (p > tmp);

            int moveLen = (int)strlen(after) + 1;
            memmove(p, after, moveLen);

            found = strstr(tmp, "..");
            if (!found)
                break;
        }

        strncpy(outBuffer, tmp, 0x100);
        outBuffer[255] = 0;
    }

    return outBuffer;
}

void CDictionaryFileParser::LoadFileIntoBuffer(const char* inFileName,
    char** outBuffer, int* outSize)
{
    DbgPrint("LoadFileIntoBuffer");
    unsigned int handle = fileOpen(inFileName, 1);
    *outBuffer = NULL;
    *outSize = 0;

    if (handle == (unsigned int)-1)
        return;

    unsigned int size = fileSize(inFileName);
    int allocSize = (int)(size + 4);
    int readSize = (int)(size + 2);
    *outSize = readSize;

    char* buf = NULL;
    if (allocSize > 0)
        buf = (char*)memAllocAlignCore(allocSize, 0, 0,
            "source/dictionary/CDictionaryFileParser.cpp", 592,
            "", 1);
    *outBuffer = buf;

    if (buf)
    {
        fileRead(handle, (unsigned __int8*)buf, readSize - 2);
        buf[readSize - 1] = 0;
        buf[readSize - 2] = 0;
    }

    fileClose(handle);
}

yy_buffer_state* yy_current_buffer_0 = NULL;
char* yy_c_buf_p_0 = NULL;
int yy_start_0 = 0;
int yy_n_chars_0 = 0;
char yy_hold_char_0 = '\0';
int yy_did_buffer_switch_on_eof_0 = 0;
_iobuf* resfilein = NULL;
char* resfiletext = NULL;

void resfile_switch_to_buffer(yy_buffer_state* new_buffer)
{
    if (yy_current_buffer_0 == new_buffer)
        return;

    if (yy_current_buffer_0)
    {
        *yy_c_buf_p_0 = yy_hold_char_0;
        yy_current_buffer_0->yy_buf_pos = yy_c_buf_p_0;
        yy_current_buffer_0->yy_n_chars = yy_n_chars_0;
    }

    yy_current_buffer_0 = new_buffer;
    yy_n_chars_0 = new_buffer->yy_n_chars;
    yy_c_buf_p_0 = new_buffer->yy_buf_pos;
    yy_did_buffer_switch_on_eof_0 = 1;
    resfilein = new_buffer->yy_input_file;
    resfiletext = yy_c_buf_p_0;
    yy_hold_char_0 = *yy_c_buf_p_0;
}

yy_buffer_state* resfile_scan_buffer(char* base, unsigned int size)
{
    if (size < 2 || base[size - 2] != 0 || base[size - 1] != 0)
        return NULL;

    DbgPrint("resfile_scan_buffer: g_scratchStack=%d size=%u base=0x%08X",
        g_scratchStack, size, (unsigned int)base);

    void* testAlloc = memAllocAlignCore(0x28, g_scratchStack, 0,
        "test", 0, "", 1);
    DbgPrint("test alloc from scratch stack = 0x%08X", (unsigned int)testAlloc);

    yy_buffer_state* buf = (yy_buffer_state*)memAllocAlignCore(
        0x28, g_scratchStack, 0,
        "d:/user/nightly/70192/ms_mar_08/engine/source/dictionary/resfilelexer.l",
        40, "", 1);

    if (!buf)
    {
        fprintf(stderr, "%s\n", "out of dynamic memory in yy_scan_buffer()");
        exit(2);
    }

    buf->yy_ch_buf = base;
    buf->yy_buf_pos = base;
    buf->yy_buf_size = size - 2;
    buf->yy_n_chars = size - 2;
    buf->yy_is_our_buffer = 0;
    buf->yy_input_file = 0;
    buf->yy_is_interactive = 0;
    buf->yy_at_bol = 1;
    buf->yy_fill_buffer = 0;
    buf->yy_buffer_status = 0;

    resfile_switch_to_buffer(buf);
    return buf;
}

bool CDictionaryFileParser::PushInputFileMem(char* buffer, int bufferLen,
    const char* inBufferName, bool deleteFileBuffer)
{
    DbgPrint("PushInputFileMem");
    SInputFile* inputFile = &m_fileStack[m_fileStackIndex];

    vafmtbuff(inputFile->m_file, 0x100, "%s", inBufferName);

    inputFile->m_fileBuffer = buffer;
    inputFile->m_fileSize = bufferLen;
    inputFile->m_deleteFileBuffer = deleteFileBuffer;
    inputFile->m_lineNo = 1;

    if (!buffer)
        return false;

   inputFile->m_lexBuffer = resfile_scan_buffer(buffer, bufferLen);

    if (!inputFile->m_lexBuffer)
    {
        if (deleteFileBuffer && buffer)
            memFreeFlags(buffer, 1);
        return false;
    }

    m_fileStackIndex++;
    return true;
}

bool CDictionaryFileParser::PushInputFile(const char* inFileName)
{
    DbgPrint("PushInputFile");
    // Get string length
    unsigned int len = (unsigned int)strlen(inFileName);
    if (len >= 0x100)
        return false;

    SInputFile* inputFile = &m_fileStack[m_fileStackIndex];

    // Build full path: filePathPrefix + inFileName
    _snprintf(inputFile->m_file, 0x100, "%s%s", m_filePathPrefix, inFileName);
    ParseFileName(inputFile->m_file, 0x100, inputFile->m_file);
    inputFile->m_file[255] = 0;

    // Copy to m_origFile if not yet set
    if (!m_origFile[0])
    {
        const char* src = inputFile->m_file;
        char* dst = m_origFile;
        do
        {
            *dst++ = *src;
        } while (*src++);
    }

    inputFile->m_lineNo = 1;
    LoadFileIntoBuffer(inputFile->m_file, &inputFile->m_fileBuffer, &inputFile->m_fileSize);
    inputFile->m_remaining = inputFile->m_fileSize;

    return PushInputFileMem(inputFile->m_fileBuffer, inputFile->m_fileSize, inFileName, true);
}

void CHashDictionary::AddBlockToFreePool(SHashDictionaryBlock* inBlock)
{
    unsigned int stride = m_freeHashEntries.objectSize & 0xFFFFFFF;

    // Thread 8 entries from the block into the free list
    // Each entry is 'stride' bytes apart, linked via first DWORD
    char* base = (char*)inBlock->entries;
    char* p = base + stride * 7;

    poolObject* prev = m_freeHashEntries.free;
    for (int i = 7; i >= 0; i--)
    {
        poolObject* entry = (poolObject*)(base + stride * i);
        entry->next = prev;
        prev = entry;
    }

    m_freeHashEntries.free = prev;
    m_freeHashEntries.freeCount += 8;
    m_freeHashEntries.totalCount += 8;

    if (inBlock->entries != (SHashEntry*)m_freeHashEntries.objects)
        m_freeHashEntries.objectSize |= 0x40000000u;
}

SHashEntry* CHashDictionary::AllocEntry(const char* inHashKey)
{
    if (!m_freeHashEntries.freeCount)
    {
        SHashDictionaryBlock* v4 = (SHashDictionaryBlock*)memAllocAlignCore(
            0x104, m_contentsHeap, 0,
            "d:/user/nightly/70192/ms_mar_08/engine/include\\engine/mem/newdelete.inl",
            6, "new with heap", 2);
        if (v4)
        {
            v4->nextBlock = m_allocatedBlocksList;
            m_allocatedBlocksList = v4;
            AddBlockToFreePool(v4);
        }
    }

    SHashEntry* free = (SHashEntry*)m_freeHashEntries.free;
    if (free)
    {
        m_freeHashEntries.free = *(poolObject**)((char*)free + 0x00);

        m_freeHashEntries.freeCount--;

        if (inHashKey)
        {
            const char* v7 = m_keyPool->Copy(inHashKey);
            if (v7)
                stringTableAddWithStorageCore(&m_stringTable, (stringTableElement*)free, v7, free);
        }
        else
        {
            *(void**)((char*)free + 0x00) = nullptr;
            *(void**)((char*)free + 0x04) = free;
            *(void**)((char*)free + 0x08) = m_unnamedValues;
            m_unnamedValues = (stringTableElement*)free;
            m_unnamedValuesCount++;
        }

        if (m_contentsTail)
            m_contentsTail->next = free;
        else
            m_contentsHead = free;

        *(void**)((char*)free + 0x1C) = nullptr;
        *(void**)((char*)free + 0x18) = m_contentsTail;
        m_contentsTail = free;
    }

    return free;
}

void CHashDictionary::DiscardElement(SHashEntry* ele)
{
    unsigned char type = *(unsigned char*)((char*)ele + 0x10);
    unsigned short flags = *(unsigned short*)((char*)ele + 0x12);

    if ((type == 8 && (flags & 1) == 0) || (type == 16 && (flags & 1) == 0))
    {
        void** ptr = (void**)*(int*)((char*)ele + 0x14);
        if (ptr)
        {
            // virtual Release call: (**vtable)(ptr, 1)
            void** vtable = (void**)*ptr;
            void(__fastcall * release)(void*, int) = (void(__fastcall*)(void*, int))vtable[0];
            release(ptr, 1);
        }
        *(int*)((char*)ele + 0x14) = 0;
    }
}

CTemplate* CHashDictionary::GetTemplate()
{
    return m_template;
}

void CHashDictionary::SetDictSourceLocation(const char* inSourceFileName, int inSourceLineNo)
{
    m_thisDictsSourceFile = m_keyPool->Copy(inSourceFileName);
    m_thisDictsSourceLine = inSourceLineNo;
}

void CHashDictionary::EnterValueWithSourceLocation(const char* inKey, unsigned char inFlags, const TDictDataValue* inValue, unsigned short inKeyLen, const char* inSourceFileName, int inSourceLineNo)
{
    // Just forward to EnterValue, ignoring source location
    EnterValue(inKey, inFlags, inValue, (short)inKeyLen);
}

void CHashDictionary::EnterValue(
    const char* inHashKey, unsigned char inType,
    const TDictDataValue* inValue, unsigned short inFlags)
{
    SHashEntry* v9 = nullptr;
    SHashEntry* v10 = nullptr;

    if (inHashKey && stringTableFind(&m_stringTable, inHashKey, (void**)&v10))
    {
        v9 = v10;
        DiscardElement(v10);
    }
    else
    {
        v9 = AllocEntry(inHashKey);
    }

    if (v9)
    {
        *(unsigned short*)((char*)v9 + 0x12) = inFlags;
        *(unsigned char*)((char*)v9 + 0x10) = inType;
        *(int*)((char*)v9 + 0x14) = inValue->i;
    }
}

bool CHashDictionary::GetOptionalValueSelf(
    const char* inKeyName, unsigned char* outType,
    TDictDataValue* outValue, unsigned short* outFlags)
{
    SHashEntry* entry = nullptr;

    if (!stringTableFind(&m_stringTable, inKeyName, (void**)&entry))
        return false;

    if (outType)
        *outType = *(unsigned char*)((char*)entry + 0x10);

    if (outValue)
        outValue->i = *(int*)((char*)entry + 0x14);

    if (outFlags)
        *outFlags = *(unsigned short*)((char*)entry + 0x12);

    return true;
}

bool CHashDictionary::GetDictSourceLocation(const char** outSourceFileName, int* outSourceLineNo)
{
    *outSourceFileName = m_thisDictsSourceFile;
    *outSourceLineNo = m_thisDictsSourceLine;
    return m_thisDictsSourceFile != nullptr;
}

void CHashDictionary::InitDictionary()
{
    m_freeHashEntries.objects = NULL;
    m_freeHashEntries.freeCount = 0;
    m_freeHashEntries.totalCount = 0;
    m_freeHashEntries.objectSize = 32;
    m_freeHashEntries.free = NULL;

    m_firstBlock.nextBlock = NULL;
    AddBlockToFreePool(&m_firstBlock);

    m_allocatedBlocksList = NULL;
    m_unnamedValues = NULL;
    m_unnamedValuesCount = 0;

    m_stringTable.numEntries = 0;
    m_stringTable.mallocedStorage = NULL;
    m_stringTable.index = m_elementPtrs;
    m_stringTable.length = 64;

    memset(m_elementPtrs, 0, 0x100);

    m_contentsTail = NULL;
    m_contentsHead = NULL;
    m_cachedElement = NULL;
    m_cachedElementIndex = -1;
}

CDictionaryFileParser* s_parser = NULL;
CDictionaryFileParser* s_Parse = NULL;
int CHashDictionary::s_objectHeap = -1;

CDictionary* CDictionaryFileParser::AllocDictionary(bool inIsForTemplateUse)
{
    DbgPrint("AllocDictionary: this = %p, m_flags = %d", this, m_flags);

    CDictionary* dict = NULL;

    if (m_allocator)
    {
        dict = m_allocator->AllocDictionary(inIsForTemplateUse);
    }
    else
    {
        CHashDictionary* hashDict = (CHashDictionary*)memAllocAlignCore(
            0x25C, CHashDictionary::s_objectHeap, 0,
            "d:/user/nightly/70192/ms_mar_08/engine/include\\engine/dictionary/CHashDictionary.h",
            144, "", 2);
        DbgPrint("hashDict: hashDict=%p s_objectHeap=%d s_heapDictContents=%d",
            hashDict, CHashDictionary::s_objectHeap, CDictionary::s_heapDictContents);
        if (hashDict)
        {
            new (hashDict) CHashDictionary();  // sets vtable pointer
            hashDict->m_keyPool = m_KeyPool;
            hashDict->m_cachedElementIndex = -1;
            hashDict->m_contentsHeap = CDictionary::s_heapDictContents;
            hashDict->m_thisDictsSourceFile = NULL;
            hashDict->m_thisDictsSourceLine = 0;
            hashDict->m_cachedElement = NULL;
            hashDict->m_template = NULL;
            hashDict->InitDictionary();
            dict = hashDict;
        }
    }

    if ((m_flags & 1) && dict && m_fileStackIndex > 0)
    {
        SInputFile* inputFile = &m_fileStack[m_fileStackIndex - 1];

        const char* file = inputFile->m_lineNo ? inputFile->m_file : m_origFile;
        dict->SetDictSourceLocation(file, inputFile->m_lineNo);
    }

    return dict;
}

CDictionary* CDictionaryFileParser::PushTemplate(char* inTemplateName)
{
    STUB();
    return nullptr;
}

CArray* CDictionaryFileParser::PushArray(int inPrimType, CTemplate* inTemplate, const char* inKey)
{
    STUB();
    return nullptr;
}

CTemplate* CDictionaryFileParser::PrevTemplate()
{
    STUB();
    return nullptr;
}

CDictionary* CDictionaryFileParser::PushDict(bool inIsTemplate)
{
    if (m_StackIndex >= 15)
        return nullptr;

    CDictionary* dict = AllocDictionary(inIsTemplate);
    if (!dict)
        return nullptr;

    m_StackIndex++;

    // Store dict at slot +0 (bytes 0-3) of the new stack level
    *(CDictionary**)((char*)m_Stack + m_StackIndex * 16 + 8) = dict;
    // Clear template slot (bytes 4-7)
    *(int*)((char*)m_Stack + m_StackIndex * 16 + 4) = 0;
    // Clear array slot (bytes 8-11)
    *(int*)((char*)m_Stack + m_StackIndex * 16 + 8) = 0;

    return dict;
}

const char* CDictionaryFileParser::CopyValueString(const char* inStr)
{
    // Calculate string length
    const char* p = inStr;
    while (*(unsigned __int8*)p++) {}
    unsigned int len = (unsigned int)(p - inStr - 1);

    CStrPool* pool = s_Parse->m_StrValuePool;

    if (len >= 0x3000)
    {
        // String too long for stack buffer — copy directly
        return pool->Copy(inStr);
    }

    // Process escape sequences: strip backslash before quote (\")
    char buf[0x3010];
    char v = *inStr;
    int  outIdx = 0;

    if (v)
    {
        do
        {
            ++inStr;
            buf[outIdx] = v;
            v = *inStr;

            // If current char is '"' and previous was '\', don't advance outIdx
            // (effectively removes the backslash from the output)
            if (v != '"' || buf[outIdx] != '\\')
            {
                ++outIdx;
            }
        } while (v);
    }

    buf[outIdx] = '\0';
    return pool->Copy(buf);
}

int CDictionaryFileParser::PopInputFile()
{
    if (m_fileStackIndex <= 0)
        return false;

    SInputFile* fi = &m_fileStack[m_fileStackIndex - 1];

    // Free the lex buffer if it exists
    if (fi->m_lexBuffer)
    {
        if (fi->m_lexBuffer == yy_current_buffer_0)
            yy_current_buffer_0 = nullptr;

        if (fi->m_lexBuffer->yy_is_our_buffer)
            memFreeFlags(fi->m_lexBuffer->yy_ch_buf, 1);

        memFreeFlags(fi->m_lexBuffer, 1);
        fi->m_lexBuffer = nullptr;
    }

    // Free the file buffer if we own it
    if (fi->m_deleteFileBuffer && fi->m_fileBuffer)
    {
        memFreeFlags(fi->m_fileBuffer, 1);
        fi->m_fileBuffer = nullptr;
    }

    m_fileStackIndex--;

    if (m_fileStackIndex <= 0)
        return 0;

    // Switch lexer back to previous file's buffer
    resfile_switch_to_buffer(m_fileStack[m_fileStackIndex - 1].m_lexBuffer);
    return 1;
}

struct StackSlot
{
    unsigned int  flags;   // +0
    CTemplate* tmpl;    // +4
    CDictionary* dict;    // +8
    CDictionary* src;     // +12
};
static StackSlot* slot(CDictionaryFileParser* p, int i)
{
    return (StackSlot*)(p->m_Stack + i * 16);
}

void CDictionaryFileParser::EnterValue(const char* inKey, unsigned __int8 inType, const TDictDataValue* inValue)
{
    CDictionaryFileParser* p = s_Parse;

    // Get current template from stack
    CTemplate* curTmpl = slot(p, p->m_StackIndex)->tmpl;

    if (curTmpl && curTmpl->m_isFieldMeta
        && _stricmp(inKey, "default") == 0
        && !(p->m_flags & 2))
    {
        // Field meta path — enter via meta field name
        int si = p->m_StackIndex;
        CDictionary* parentDict = nullptr;
        if (si > 0)
            parentDict = *(CDictionary**)((char*)&p->m_flags + si * 16);

        const char* metaFieldName = curTmpl->GetMetaDataFieldName();

        if (parentDict)
        {
            parentDict->EnterValue(metaFieldName, inType, inValue, 0);
        }
    }
    else
    {
        // Normal path — enter into current stack dict
        // Get source file/line from file stack
        int   sourceLine = -1;
        const char* sourceFile = nullptr;

        int fidx = p->m_fileStackIndex;
        if (fidx > 0)
        {
            SInputFile* fi = &p->m_fileStack[fidx - 1];
            sourceLine = fi->m_lineNo;
            sourceFile = fi->m_file;
        }
        else
        {
            sourceFile = p->m_origFile;
        }

        // Get current container from stack (+2 levels up for the actual dict)
        int stackOffset = (p->m_StackIndex + 2) * 16;
        CDictionary* dict = *(CDictionary**)((char*)&p->m_flags + stackOffset);

        if (dict);
            dict->EnterValueWithSourceLocation(inKey, inType, inValue, 0,
                sourceFile, sourceLine);
    }
}
extern int resfileparse();

CDictionary* CDictionaryFileParser::DoParse()
{
    DbgPrint("DoParse: this = %p, m_flags = %d", this, m_flags);

    CDictionary* dict = AllocDictionary(false);
    DbgPrint("AllocDictionary returned %p", dict);
    *(CDictionary**)m_Stack = dict;

    if (!m_rootDictSourceLineSet && dict && m_fileStackIndex > 0)
    {
        m_rootDictSourceLineSet = true;
        dict->SetDictSourceLocation(m_fileStack[0].m_file, 0);
    }

    s_Parse = this;
    s_parser = this;
    int parseResult = resfileparse();

    DbgPrint("parseResult = %d", parseResult);

    s_Parse = NULL;
    s_parser = NULL;

    if (parseResult != 0)
        return NULL;

    CDictionary* result = *(CDictionary**)m_Stack;
    DbgPrint("result = %p, m_Stack[0] = %p", result, *(void**)m_Stack);
    *(CDictionary**)m_Stack = NULL;
    return result;
}