#pragma once

class CStrPool;
class CDictionary;
class CDictionaryAllocator;
class CBaseObject;

enum ESerialiseMode : __int32
{
	k_serialiseModeLoad = 0x0,
	k_serialiseModeSave = 0x1,
	k_serialiseModeRuntimeLoad = 0x2,
	k_serialiseModeRuntimeSave = 0x3,
	k_serialiseModeNetworkLoad = 0x4,
	k_serialiseModeNetworkSave = 0x5,
};

class __declspec(align(4)) CEngineSerialiseStore
{
public:
	virtual ~CEngineSerialiseStore() {}
	virtual void InformKeyBeingRead(const char* key) {}
	virtual void AttachToDictionaryForLoad(CDictionary* inDict, CStrPool* inKeyPool, CStrPool* inStrValuePool, CDictionaryAllocator* inAllocator, ESerialiseMode mode);
	virtual void AttachToDictionaryForSave(CDictionary* inDict, CStrPool* inKeyPool, CStrPool* inStrValuePool, CDictionaryAllocator* inAllocator, ESerialiseMode mode);
	virtual void BeginDictSerialise(CEngineSerialiseStore*) {}
	virtual CBaseObject* GetThisObject() { return nullptr; }
	virtual void SetThisObjectsKeyCopy(const char*) {}

	ESerialiseMode m_mode;
	CStrPool* m_strValuePool;
	CStrPool* m_keyStrPool;
	CDictionary* m_dict;
	CEngineSerialiseStore* m_parentStore;
	const char* m_thisObjectKey;
	CDictionaryAllocator* m_allocator;
	bool m_stackDictSerialiseInProgress;
};