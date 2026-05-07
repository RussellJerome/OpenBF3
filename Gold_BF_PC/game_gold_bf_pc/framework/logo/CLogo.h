#pragma once
#include "framework/object/CBaseObject.h"

class CLogo : public CBaseObject
{
public:
	virtual const char* ClassID() override
	{
		return "Logo";
	};

	char m_m0vBigEndianFileName[64];
	char m_m0vLittleEndianFileName[64];
	char m_soundId[32];
};

//So there are a billion of these functions. I have yet to determine if they are unique or if we can combine them into 1 template function
CBaseObject* CreateObOfClass_CLogo(const char* inClassID);