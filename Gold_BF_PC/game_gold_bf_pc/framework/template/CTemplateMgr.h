#pragma once
#include "framework/template/CTemplate.h"

class CStrPool;
class CTemplate;
class CDictionary;

class CTemplateMgr
{
public:
	CStrPool* m_keyPool;
	CFixedSizeStringTableT<CTemplate, 14500> m_hash;

	virtual ~CTemplateMgr() {};

	CTemplateMgr(CStrPool*);

	CTemplate* GetTemplate(const char*);
	CTemplate* GetTemplateRecursive(const char*);
	CTemplate* AddTemplate(const char*, CDictionary*);

	static CTemplateMgr* s_mgr;
};

extern CHashStrPool* s_keyPool_2;