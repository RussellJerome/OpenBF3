#include "CLogo.h"
#include "framework/object/CObjectAllocator.h"

CBaseObject* CreateObOfClass_CLogo(const char* inClassID)
{
    void* mem = s_allocator->AllocObject(0xB4, inClassID);
    if (!mem)
        return nullptr;

    CLogo* obj = (CLogo*)mem;

    obj->m_saveTemplate = nullptr;
    obj->m_componentCachePtr = nullptr;
    obj->m_baseFlags = 0;
    obj->m_valid = 0xBEEFFACE;

    memset((char*)obj + 0x14, 0, 0x40);
    memset((char*)obj + 0x54, 0, 0x40);
    *(unsigned char*)((char*)obj + 0x94) = 0;

    return obj;
}