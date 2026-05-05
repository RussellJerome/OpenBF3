#include "CEngineSerialiseStore.h"

void CEngineSerialiseStore::AttachToDictionaryForLoad(CDictionary* inDict, CStrPool* inKeyPool, CStrPool* inStrValuePool, CDictionaryAllocator* inAllocator, ESerialiseMode mode)
{
	m_mode = mode;
	m_keyStrPool = inKeyPool;
	m_strValuePool = inStrValuePool;
	m_dict = inDict;
	m_allocator = inAllocator;
	m_parentStore = 0;
	m_thisObjectKey = 0;
}

void CEngineSerialiseStore::AttachToDictionaryForSave(CDictionary* inDict, CStrPool* inKeyPool, CStrPool* inStrValuePool, CDictionaryAllocator* inAllocator, ESerialiseMode mode)
{
	m_mode = mode;
	m_keyStrPool = inKeyPool;
	m_strValuePool = inStrValuePool;
	m_dict = inDict;
	m_allocator = inAllocator;
	m_parentStore = 0;
	m_thisObjectKey = 0;
}
