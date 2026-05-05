#pragma once
#include "engine/string/engstring.h"
#include "util/unorgtypes.h"
#include "engine/mem.h"

class CEmbeddedResFile
{
public:
    CEmbeddedResFile(const SEmbeddedResFileHeader* header, void* memory);

    unsigned __int8* m_buffer;
    unsigned int m_bufferSize;
    void* m_memory;
    unsigned int m_memHandle;
    CEmbeddedStrPool m_keyPool;
    CEmbeddedStrPool m_strValuePool;
    embeddedPreloadEntry* m_preloadList;
    unsigned int m_preloadListSize;
    unsigned int m_animInfoOffset;

    static bool m_allowRuntimeEmbed;
    static CEmbeddedResFile* m_templates;
};

class CEmbeddedResFileInt
{
public:
    static void Setup(CEmbeddedResFile*, const SEmbeddedResFileHeader*);
};

extern poolState s_embeddedResFilePool;