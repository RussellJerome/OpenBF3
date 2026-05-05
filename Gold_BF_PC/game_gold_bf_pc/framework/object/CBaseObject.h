#pragma once

class CTemplate;
class CComponentCache;
class CSerialiseStore;
class CComponent;

class CBaseObject
{
public:
	virtual ~CBaseObject() {}
	virtual const char* ClassID() { return nullptr; }
	virtual unsigned int ObjectTypeID() { return 0; }
	virtual bool IsOfObjectType(unsigned int Type) { return false; }
	virtual const char* GetSaveTemplateName() { return nullptr; }
	virtual CTemplate* GetSaveTemplate() { return nullptr; }
	virtual void Serialise(CSerialiseStore* ioStore);
	virtual CComponent* GetComponentSelf(unsigned int inID) { return nullptr; }

	CTemplate* m_saveTemplate;
	CComponentCache* m_componentCachePtr;
	unsigned __int8 m_baseFlags;
	int m_valid;

	static const char* s_baseFlagsStrings[6];
};
