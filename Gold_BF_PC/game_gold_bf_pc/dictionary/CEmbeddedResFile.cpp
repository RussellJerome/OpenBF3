#include "CEmbeddedResFile.h"
#include "util/powerpc.h"
#include "engine/mem.h"
#include "framework/template/CTemplate.h"

poolState s_embeddedResFilePool;

void embeddedStructureInitRecursive(CEmbeddedResFile* resFile, CEmbeddedDictionary* data)
{
    unsigned int sentinel = *(unsigned int*)data;

    if (sentinel == 8)
    {
        // Uninitialized CEmbeddedDictionary — construct it in place
        new (data) CEmbeddedDictionary(resFile);

        int numValues = data->CountValues();
        for (int i = 0; i < numValues; i++)
        {
            const char* key = nullptr;
            unsigned __int8  type = 0;
            TDictDataValue   value = {};
            unsigned __int16 flags = 0;

            data->GetNthValue(i, &key, &type, &value, (short*)&flags);

            if (type == 16 || type == 8)
                embeddedStructureInitRecursive(resFile, (CEmbeddedDictionary*)value.d);
        }
    }
    else if (sentinel == 16)
    {
        // Uninitialized CEmbeddedArray — set pool index byte then init
        CEmbeddedArray* arr = (CEmbeddedArray*)data;

        *((unsigned __int8*)arr + 5) = (unsigned __int8)
            (((char*)resFile - (char*)s_embeddedResFilePool.objects) / sizeof(CEmbeddedResFile));

        unsigned __int8 elementType = 0;
        CTemplate* elementTemplate = nullptr;
        arr->GetArrayType(&elementType, &elementTemplate);

        if (elementType == 16 || elementType == 8)
        {
            int count = arr->CountItems();
            for (int j = 0; j < count; j++)
            {
                TDictDataValue   item = {};
                unsigned __int16 flags = 0;
                arr->GetNthItem(j, &item, &flags);

                embeddedStructureInitRecursive(resFile, (CEmbeddedDictionary*)item.d);
            }
        }
    }
}

void CEmbeddedResFileInt::Setup(CEmbeddedResFile* self, const SEmbeddedResFileHeader* header)
{
    self->m_bufferSize = header->resFileDataSize;
    self->m_buffer = (unsigned __int8*)header->resFileData;

    self->m_keyPool.m_buffer = (char*)header->keyPool;
    self->m_keyPool.m_nextStringPos = (char*)header->keyPool;
    self->m_keyPool.m_bufferSize = header->keyPoolSize;

    self->m_strValuePool.m_buffer = (char*)header->strValuePool;
    self->m_strValuePool.m_nextStringPos = (char*)header->strValuePool;
    self->m_strValuePool.m_bufferSize = header->strValuePoolSize;

    self->m_preloadList = header->preloadList;
    self->m_preloadListSize = header->preloadListSize;
    self->m_animInfoOffset = header->animInfoOffset;

    CEmbeddedResFile* templates = CEmbeddedResFile::m_templates;
    if (templates)
    {
        self->m_keyPool.m_templateStrPool = &templates->m_keyPool;
        self->m_strValuePool.m_templateStrPool = &templates->m_strValuePool;
    }

    embeddedStructureInitRecursive(self, (CEmbeddedDictionary*)self->m_buffer);

    if (self->m_animInfoOffset != (unsigned int)-1)
        embeddedStructureInitRecursive(self, (CEmbeddedDictionary*)&self->m_buffer[self->m_animInfoOffset]);
}

CEmbeddedResFile::CEmbeddedResFile(const SEmbeddedResFileHeader* header, void* memory)
{
    // Zero initialise pools
    m_keyPool.m_buffer = nullptr;
    m_keyPool.m_bufferSize = 0;
    m_keyPool.m_nextStringPos = nullptr;
    m_keyPool.m_stringTable = nullptr;
    m_keyPool.m_templateStrPool = nullptr;

    m_strValuePool.m_buffer = nullptr;
    m_strValuePool.m_bufferSize = 0;
    m_strValuePool.m_nextStringPos = nullptr;
    m_strValuePool.m_stringTable = nullptr;
    m_strValuePool.m_templateStrPool = nullptr;

    m_memHandle = 0;
    m_memory = memory;
    m_preloadList = nullptr;
    m_preloadListSize = 0;
    m_animInfoOffset = (unsigned int)-1;

    if (memory)
    {
        CEmbeddedResFileInt::Setup(this, header);
        return;
    }

    // Allocate and copy resFileData blob
    m_bufferSize = header->resFileDataSize;
    if (header->resFileDataSize > 0)
        m_buffer = (unsigned __int8*)memAllocAlignCore(
            header->resFileDataSize, 0, 0,
            "source/dictionary/CEmbeddedResFile.cpp", 171, "", 1);
    else
        m_buffer = nullptr;

    // Allocate keyPool
    if (header->keyPoolSize > 0)
        m_keyPool.m_buffer = (char*)memAllocAlignCore(
            header->keyPoolSize, 0, 0,
            "source/dictionary/CEmbeddedResFile.cpp", 172, "", 1);
    else
        m_keyPool.m_buffer = nullptr;

    m_keyPool.m_nextStringPos = m_keyPool.m_buffer;
    m_keyPool.m_bufferSize = header->keyPoolSize;

    // Allocate strValuePool
    if (header->strValuePoolSize > 0)
        m_strValuePool.m_buffer = (char*)memAllocAlignCore(
            header->strValuePoolSize, 0, 0,
            "source/dictionary/CEmbeddedResFile.cpp", 173, "", 1);
    else
        m_strValuePool.m_buffer = nullptr;

    m_strValuePool.m_nextStringPos = m_strValuePool.m_buffer;
    m_strValuePool.m_bufferSize = header->strValuePoolSize;

    // Copy data blobs
    memcpy(m_buffer, header->resFileData, header->resFileDataSize);
    memcpy(m_keyPool.m_buffer, header->keyPool, header->keyPoolSize);
    memcpy(m_strValuePool.m_buffer, header->strValuePool, header->strValuePoolSize);

    // Initialise entries
    if (header->numEntries > 0)
    {
        int remaining = header->numEntries;
        int idx = 0;

        do
        {
            unsigned int offsetAndType = bswap32(header->entries[idx].offsetAndType);
            unsigned int offset = offsetAndType & 0x3FFFFFF;
            unsigned int type = offsetAndType >> 26;

            unsigned __int8* ptr = &m_buffer[offset];

            if (type == 8)
            {
                if (ptr)
                    new (ptr) CEmbeddedDictionary(this);
            }
            else if (type == 16)
            {
                if (ptr)
                    new (ptr) CEmbeddedArray(this);
            }

            --remaining;
            ++idx;
        } while (remaining);
    }

    CEmbeddedResFile::m_templates = this;
}

bool CEmbeddedResFile::m_allowRuntimeEmbed = true;
CEmbeddedResFile* CEmbeddedResFile::m_templates = nullptr;