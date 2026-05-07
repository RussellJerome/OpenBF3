#pragma once
#include "engine/string/engstring.h"

class poolState;
class dlinkdef_s;
class CBaseObject;

class CObjectAllocator
{
public:
	struct TState
	{
		unsigned __int16* table;
		poolState* poolOfPools;
		dlinkdef_s* pools;
		unsigned int poolCount;
		unsigned int poolSize;
		unsigned __int16* sizes;
		int sizesCount;
		int heap;
		void* freePtr;
		void* endPtr;
		unsigned int tableSize;
		unsigned int seedNum;
	};

	virtual ~CObjectAllocator();

	CObjectAllocator(int inHeap, unsigned int inId, unsigned int seedNum, int flags);

	void AddNewSize(unsigned short alignedSize, const char* inClassID);
	void RegisterClassNoKeyCopy(unsigned int inSize, unsigned int inProxySize, const char* inClassID);
	void* AllocObject(int inSize, const char* inClassID);
	void FreeObject(void* inPtr);

	int m_maxObjectSize;
	CObjectAllocator::TState m_state;
	int m_flags;
	unsigned int m_id;
	int m_totalAlloc;
	int m_peakAlloc;
	int m_fixedSizeIntervalEnd;
	int m_fixedSizeIntervalAlign;
	CObjectAllocator* m_fallbackAllocator;
};

struct SObjectCreator
{
	stringTableElement ele;
	CBaseObject* (__cdecl* creatorFunc)(const char*);
	CBaseObject* (__cdecl* proxyCreatorFunc)(const char*);
	unsigned int objectTypeID;
};

class CObjectFactory
{
public:
	CBaseObject* CreateObject(const char* inClassID, bool useProxy);
	void RegisterClassWithProxyNoKeyCopy(
		const char* inClassID,
		unsigned int     objectTypeID,
		CBaseObject* (*inCreatorFunc)(const char*),
		unsigned int     inObjectSize,
		const char* inProxyClassID,
		unsigned int     proxyObjectTypeID,
		CBaseObject* (*inProxyCreatorFunc)(const char*),
		unsigned int     inProxyObjectSize);
public:
	~CObjectFactory() {}
	CObjectAllocator* m_allocator;
	CFixedSizeStringTableT<SObjectCreator, 1025> m_Hash;

	static CObjectFactory* s_Factory;
};

extern CObjectAllocator* s_allocator;