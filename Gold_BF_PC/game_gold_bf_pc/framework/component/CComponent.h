#pragma once
#include "../object/CBaseObject.h"
#include "util/unorgtypes.h"

struct CollisionInfo
{
	unsigned int g1;
	unsigned int g2;
	unsigned int pmh1;
	unsigned int pmh2;
	struct opaqueProp* propHandle1;
	struct opaqueProp* propHandle2;
	vec3_u hitpos;
	vec3_u hitnorm;
	float impactvel;
	int meshIdx;
};

class CGameProp;

class CComponent : public CBaseObject
{
};

class CPropComponent : public CComponent
{
public:
	CGameProp* m_prop;
};

class CTickingComponent : public CPropComponent
{
public:

	virtual void TickingComponentTick(float dt);
	virtual void OnCollision(CGameProp* prop, CollisionInfo* info);
	virtual void CalcMatrices(CGameProp* prop, double timeSlice);

	virtual unsigned int ObjectTypeID() override
	{
		return 0x7469636B;
	}

	virtual bool IsOfObjectType(unsigned int inId) override
	{
		return inId == 0x7469636B;
	}
	//CTickingComponent::InsertIntoWorld
};

class CComponentList : public CTickingComponent
{
public:
	struct SComponentAndName
	{
		CTickingComponent* component;
		char serialiseName[32];
	};

	int m_numComponents;
	CComponentList::SComponentAndName m_componentsAndNames[10];

	void Free();
	virtual CTickingComponent* GetComponentSelf(unsigned int inComponentType) override;
	virtual void TickingComponentTick(float dt) override;
	virtual void OnCollision(CGameProp* prop, CollisionInfo* info) override;
	void TickingComponentCalcMatrices(float dt);

	virtual unsigned int ObjectTypeID() override
	{
		return 0x6C697374;
	}

	virtual bool IsOfObjectType(unsigned int inId) override
	{
		if (inId == 0x6C697374)
			return true;
		if (inId == 0x7469636B)
			return true;
		return false;
	}

	virtual const char* ClassID() override
	{
		return "ticking component list";
	}
};