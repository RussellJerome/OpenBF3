#pragma once

void DbgPrint(const char* format, ...);

#define STUB() DbgPrint("STUB: %s on %s", __FUNCTION__, typeid(*this).name())
#define STUB_STATIC() DbgPrint("STUB: %s", __FUNCTION__)
