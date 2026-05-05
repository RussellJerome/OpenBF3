#pragma once
#include <windows.h>
#include <iostream>

struct packfilehdr
{
    char m_magic[4];
    unsigned int m_ftofs;
    unsigned int m_ftsize;
    unsigned int m_ofofs;
    unsigned int m_ofsize;
    unsigned __int16 m_chunks;
    unsigned __int16 m_seed;
};

struct packfileft
{
    unsigned int m_filesize;
    unsigned int m_packedsize;
    unsigned int m_chunkpos;
    unsigned __int32 m_chunk : 8;
    unsigned __int32 m_compchunkoffset : 24;
};

struct packfileinfo
{
    char m_name[256];
    unsigned int m_fh[50];
    unsigned int m_fsh[50];
    unsigned __int8 m_inmem;
    unsigned __int8 m_used;
    unsigned __int8 m_initialised;
    unsigned __int8 m_needsClose;
    packfilehdr* m_pf;
    void* m_cfs[50];
    unsigned int m_size[50];
    char* m_st;
    int* m_sto;
    unsigned int* m_ht;
    packfileft* m_ft;
    unsigned __int8* m_ot;
    int m_stls;
    int m_refCount;
};

struct fileInfo
{
    unsigned int m_fh;
    int m_pack;
    int m_packfilenum;
    int m_ofs;
};

enum epakMode : __int32
{
    PAK_RAW = 0x0,
    PAK_HEADTAIL = 0x1,
    PAK_TAIL = 0x2,
};

enum ECachingType : __int32
{
    NO_CACHE = 0x0,
    CACHE_COPY = 0x1,
    CACHE_RELOOKUP = 0x2,
    CACHE_DECOMP = 0x4,
    CACHE_DONE = 0x8,
    CACHE_RC = 0x3,
    CACHE_D_C = 0x9,
    CACHE_DRC = 0x7,
    CACHE_F__C = 0x9,
    CACHE_F_RC = 0xB,
    CACHE_FD_C = 0xD,
    CACHE_FDRC = 0xF,
};

struct tpak
{
    unsigned int count;
    unsigned __int8* phead;
    unsigned __int8* ptail;
    unsigned __int8* pdestination;
    int offset;
    int offsetStart;
    int offsetEnd;
    int length;
    int priority;
    int headOffset;
    int headLength;
    unsigned __int8* pheadBuffer;
    int tailOffset;
    int tailLength;
    unsigned __int8* ptailBuffer;
    unsigned int handle;
    epakMode mode;
    int decomp;
    packfileinfo* packStruct;
    packfileft* fileStruct;
    unsigned __int8 immediate;
    unsigned __int8 disablePartialRead;
    volatile ECachingType cacheMe;
};

struct tpakHead
{
    unsigned int used;
    unsigned __int8* pbuffer;
};

struct pakIpak
{
    void* ptaskArgs;
    void* ptaskID;
    unsigned int handleCounter;
    unsigned int handles[64];
    volatile int returnCode[64];
    tpakHead head[8];
};

struct StreamInfo_s
{
    _OVERLAPPED ovr;
    void* h;
    int baseoffset;
    int pakOffset;
    int upper;
    unsigned __int8 opensingle;
    int handle;
    int lastread;
    int toread;
    int gameframe;
    int streamstate;
    char* buffer;
    tpak pak;
    StreamInfo_s* prev;
    StreamInfo_s* next;
};

extern packfileinfo s_packfiles[16];
extern fileInfo s_fileInfos[64];
extern _iobuf* s_fileHandles[64];
extern tpak pak;
extern pakIpak ipak;
extern BYTE s_fileAccessAllowed;
extern unsigned int  s_threadJobQueue;
extern StreamInfo_s  s_Streams[300];
extern StreamInfo_s  s_ActiveStreams;
extern StreamInfo_s* s_CurStream;

int findFreePackfileSlot();
int packfileOpenAtIdx(const char* filename, unsigned __int8 loadmem, int idx);
unsigned int fileOpen(const char* filename, int openflags);
int fileSize(const char* filename);
unsigned int fileRead(unsigned int handle, unsigned __int8* buffer, unsigned int length);
int fileClose(unsigned int handle);
unsigned __int8 pakInitialise();
void fileTick();