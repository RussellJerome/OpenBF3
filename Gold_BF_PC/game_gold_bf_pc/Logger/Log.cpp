#include "Log.h"
#include <windows.h>
#include <iostream>

static FILE* g_logFile = nullptr;

void DbgPrint(const char* format, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    std::cout << buffer << std::endl;

    if (!g_logFile)
        g_logFile = fopen("debug_log.txt", "w");

    if (g_logFile)
    {
        fprintf(g_logFile, "%s\n", buffer);
        fflush(g_logFile);  // flush every line so you get it even on crash
    }
}