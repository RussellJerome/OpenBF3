#include "CTemplateMgr.h"
#include "engine/string/engstring.h"
#include "engine/mem.h"
#include "Logger/Log.h"

CTemplateMgr::CTemplateMgr(CStrPool* inPool)
{
    m_keyPool = s_keyPool_2;

    // Initialise all CTemplate entries in the hash table
    for (int i = 0; i < 14500; i++)
        new (&m_hash.m_Entries[i]) CTemplate();

    // Point each ElementPtr to its corresponding CTemplate's ele field
    for (int i = 0; i < 14500; i++)
        m_hash.m_ElementPtrs[i] = &m_hash.m_Entries[i].ele;

    // Initialise string table
    m_hash.m_UsedEntries = 0;
    m_hash.m_StringTable.numEntries = 0;
    m_hash.m_StringTable.length = 14500;
    m_hash.m_StringTable.mallocedStorage = 0;
    m_hash.m_StringTable.index = m_hash.m_ElementPtrs;
    memset(m_hash.m_ElementPtrs, 0, sizeof(m_hash.m_ElementPtrs));

    // Set singleton
    CTemplateMgr::s_mgr = this;

    // Allocate a minimal CDictionary for the "field" meta template
    CTemplateOnlyDictionary* fieldDict = (CTemplateOnlyDictionary*)memAllocAlignCore(
        sizeof(CTemplateOnlyDictionary), 0, 0,
        "d:/user/nightly/70192/ms_mar_08/engine/include\\engine/boss/common.h",
        589,
        "memalloc no group",
        2);

    CDictionary* dictArg = nullptr;
    if (fieldDict)
    {
        new (fieldDict) CTemplateOnlyDictionary();
        dictArg = (CDictionary*)fieldDict;
    }

    // Register the base "field" meta template
    CTemplate* fieldTemplate = AddTemplate("field", dictArg);
    if (fieldTemplate)
        fieldTemplate->m_isFieldMeta = 1;
}

CTemplate* CTemplateMgr::AddTemplate(const char* inKey, CDictionary* inDict)
{
    // Check if template already exists
    void* existing = nullptr;
    stringTableFind(&m_hash.m_StringTable, inKey, &existing);
    if (existing)
    {
        // Already defined - log warning but continue
        char warnBuf[512];
        snprintf(warnBuf, sizeof(warnBuf),
            "tried to redefine template %s\n", inKey);
        DbgPrint(warnBuf);
    }

    // Check if hash table is full (max 14500 entries)
    if ((int)m_hash.m_UsedEntries >= 14500)
        return nullptr;

    // Copy the key string into the key pool
    const char* keyCopy = m_keyPool->Copy(inKey);

    // Get next free slot in the fixed size table
    CTemplate* slot = nullptr;

    if (m_hash.m_UsedEntries < 14500)
    {
        int idx = m_hash.m_UsedEntries;
        m_hash.m_UsedEntries++;
        slot = &m_hash.m_Entries[idx];
        stringTableAddWithStorageCore(
            &m_hash.m_StringTable,
            &m_hash.m_Entries[idx].ele,  // the ele field inside the CTemplate entry
            keyCopy,
            slot);
    }

    if (slot)
    {
        slot->Init(inDict);
        return slot;
    }

    return nullptr;
}

CHashStrPool* s_keyPool_2 = nullptr;

CTemplateMgr* CTemplateMgr::s_mgr = nullptr;