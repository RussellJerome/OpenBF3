#pragma once
#include <windows.h>

static inline unsigned int   bswap32(unsigned int v) { return _byteswap_ulong(v); }
static inline unsigned short bswap16(unsigned short v) { return _byteswap_ushort(v); }

// Use this wherever you read a big-endian DWORD out of the offset table (m_ot)
static inline unsigned int otReadU32(const unsigned __int8* p)
{
    return bswap32(*(unsigned int*)p);
}