#pragma once
#include <d3d9.h>
#include <d3dcompiler.h>
#include "util/unorgtypes.h"
#include "engine/mem.h"

//DIRECT X 9

extern IDirect3D9* g_pD3D;
extern IDirect3DDevice9* g_pd3dDevice;
extern IDirect3DTexture9* g_pD3dFrontBufferTex;

extern int s_initScreenWidth;
extern int s_initScreenHeight;
extern unsigned int s_bUseMultisampleTwo;
extern unsigned int s_bVerticalSync;
extern unsigned int s_texHwD3dFrontBuffer;
extern IDirect3DSurface9* g_pD3dBackBufferTarget;
extern IDirect3DSurface9* g_pD3dDepthStencilTarget;
extern void* s_baseAddr;

extern texdefHW* s_pTexturesHW;
extern unsigned __int8* s_pTexturesHWUsed;
extern windowdef_s windowfullscreen;
extern rStateBlock s_toSend;
struct __declspec(align(4)) TDirEntry
{
    TDirEntry* next;
    TDirEntry* parent;
    TDirEntry* children;
    void* user;
    unsigned __int16 rollingcount;
    char name[76];
};

extern TDirEntry  s_dirPoolData[15000];
extern poolState  s_dirPool;
extern TDirEntry  s_root;

enum TexFunc : __int32
{
    TEX_FUNC_MODULATE_RGBA = 0x0,
};

struct dlContextData
{
    int primitiveType;
    int numvertexes;
    int type;
    int vertexSize;
    float* startPtr;
};

struct dlContextStruct
{
    dlContextData* data;
    float* ptr;
};

struct debugdrawComm_s
{
    unsigned __int8 command;
    unsigned __int8 pad;
    unsigned __int16 extra;
};

enum debugdrawBlendMode : __int32
{
    dd_BlendOff = 0x0,
    dd_BlendOne = 0x1,
    dd_BlendAlpha = 0x2,
};

struct debugdrawRegisters_s
{
    debugdrawComm_s command;
    float colour[4];
    debugdrawBlendMode blendMode;
    int pointCount;
    unsigned __int32 : 31;
    __int32 mode3d : 1;
};

struct debugdrawBuffer_s
{
    debugdrawRegisters_s regs;
    int bufferSize;
    unsigned __int8* bufferBase;
    unsigned __int8* bufferCur;
    unsigned __int8* bufferMax;
};

struct vbinfo
{
    IDirect3DVertexBuffer9* m_vb;
    int               m_vt;
};

struct dlVertexDataInfo
{
    unsigned int sizeInFloats;
    unsigned __int8 numElems;
    unsigned __int8 type;
    unsigned __int8 set;
    char texUnit;
};

struct ibinfo
{
    IDirect3DIndexBuffer9* m_ib;
};

enum PBufferOptionType : __int32
{
    PBUFFER_END_OPTIONS = 0x0,
    PBUFFER_FLOAT_BUFFER = 0x1,
    PBUFFER_SHARE_BUFFER = 0x2,
    PBUFFER_CREATE_COLOR_TEXTURE = 0x3,
    PBUFFER_CREATE_DEPTH_TEXTURE = 0x4,
    PBUFFER_DEPTH_ONLY = 0x5,
    PBUFFER_DEPTH_COMPARE = 0x6,
    PBUFFER_STENCIL_BITS = 0x7,
    PBUFFER_DEPTH_BITS = 0x8,
    PBUFFER_USE_TEX2D = 0x9,
    PBUFFER_DISABLE_FILTERING = 0xA,
    PBUFFER_COLOR_TEXTURE_DISABLE_FILTERING = 0xB,
    PBUFFER_DEPTH_TEXTURE_DISABLE_FILTERING = 0xC,
    PBUFFER_TEXTURE_CLAMP = 0xD,
    PBUFFER_LUM_BUFFER = 0xE,
    PBUFFER_MULTISAMPLE = 0xF,
    PBUFFER_XENON_NOT_OVERLAPING_DL_PBUFFER_FRAME = 0x10,
    PBUFFER_NUM_OPTIONS = 0x11,
};

enum TexFlags : __int32
{
    TEX_FLAG_MIPMAP = 0x4,
    TEX_FLAG_DEPTH = 0x8,
    TEX_FLAG_NEAREST = 0x10,
    TEX_FLAG_FORCE_BLACK_AS_ALPHA = 0x20,
    TEX_FLAG_COMPRESSED = 0x40,
    TEX_FLAG_LOAD_FROM_ID = 0x80,
    TEX_FLAG_PERMANENT = 0x100,
    TEX_FLAG_CAN_REDUCE = 0x200,
    TEX_FLAG_RENDER_TEXTURE = 0x400,
    TEX_FLAG_RENDER_HDR = 0x800,
    TEX_FLAG_RENDER_HDR_LUM = 0x1000,
    TEX_FLAG_CLAMPU = 0x2000,
    TEX_FLAG_CLAMPV = 0x4000,
    TEX_FLAG_CLAMPUV = 0x6000,
    TEX_FLAG_LINEAR = 0x8000,
    TEX_FLAG_DONT_STREAM = 0x10000,
    TEX_FLAG_CELL_MAIN_MEM = 0x20000,
    TEX_FLAG_STREAM_HIGH = 0x40000,
    TEX_FLAG_STREAM_FIRST = 0x80000,
    TEX_FLAG_TINY_PRELOAD = 0x100000,
    TEX_FLAG_NO_PRELOAD = 0x200000,
    TEX_FLAG_CUBEMAP = 0x400000,
    TEX_FLAG_CUBEMAP_IS_LDR = 0x800000,
    TEX_FLAG_CUBEMAP_IS_HDR = 0x1000000,
    TEX_FLAG_XENON_FRONTBUFFER = 0x2000000,
    TEX_FLAG_DONT_EXPAND_ASSET_PATH = 0x8000000,
};


struct PBufferOption
{
    PBufferOptionType type;
    int value;
};

struct PBufferHW_s
{
    IDirect3DSurface9* rendertarget;
    IDirect3DSurface9* renderdepthstencil;
    D3DFORMAT          renderformat;
    D3DFORMAT          depthformat;
    unsigned __int8    bColourResolvedOnLastDeactivate;
    unsigned __int8    bDepthResolvedOnLastDeactivate;
    unsigned __int8    bColorRenderedAfterLastLastActivate;
    unsigned __int8    bDepthRenderedAfterLastLastActivate;
};

struct circularBuffer
{
    unsigned __int8* base;
    unsigned int currentPos;
    unsigned int size;
    unsigned int startFrame;
};

struct viewcache
{
    viewdef_s* view;
    vec3_u dirx;
    vec3_u diry;
    vec3_u dirz;
    float n;
    quaternion_u stn;
};

void InitD3D();
void RenderTest();
void dlPush2D();

void platformClearBufferHW(int buffermask);
void platformSwapBuffersHW();
void hwInitialise();
void rStateWrite();

int dlVBCreate(int num, int vt);
int dlIBCreate(unsigned int sizeinbytes);

void fileTableRemove(unsigned int handle);
int fileTableGetFilename(char* buffer, unsigned int handle);
int fileStreamingClose(unsigned short streamhandle);
unsigned int texLoadTextureName(const char* filename, int baseflags);
unsigned int pbufferCreate(int w, int h, int bpc, const PBufferOption* options);
int generatedecl(D3DVERTEXELEMENT9* decl, int type);

void dlFillRectangleZ(float x1, float y1, float x2, float y2, float z,
    float cr, float cg, float cb, float ca);
void platformColourMaskHW(int r, int g, int b, int a);
unsigned int texgenCreateCircleMap(
    int size, int channels,
    float radius, float gradientOffset,
    const float* col1, const float* col2,
    unsigned int flags);

unsigned int texCreateTexture2DExCore(
    unsigned int th,
    unsigned int sizeX, int sizeY,
    int numChannels, unsigned __int8* data,
    unsigned int flags, int inMipCount, int checkSize);

void dlTextureRectangleZ(
    float x1, float y1,
    float x2, float y2,
    float z,
    float u1, float v1,
    float u2, float v2,
    float cr, float cg, float cb, float ca);

void camDrawFullScreenQuadDebug(
    float offsetX, float offsetY,
    float maxX, float maxY,
    float maxU, float maxV);

void rStatePop();
void platformBlendHW(int value);
void frameResetRenderState();
void camRenderAllViews();

int vecnormaliseGold(vec3_u* vout, const vec3_u* vin);
void pbufferReleaseHW(unsigned int handle);
void pbufferActivate(unsigned int handle, unsigned int activateFlags);
void texSelectTextureEx(int handle, TexFunc func);
void debugdrawInitialise(int BufferSize);

void platformPopMatricesHW();

void BeginScene();
void EndScene();
void idrawPolygon2D(const vec4_u* colour, int numpoints, vec2_u* points);
void platformScissorGL(int x, int y, int width, int height);
void dlScissorTest(unsigned char enable);
void dlEndMain(dlContextData* dlData, float* dlPtr);
float* dlBeginMain(dlContextStruct* dl, int primitiveType, int numvertices, int type);

void idrawQuad2D(const vec4_u* colour, float x, float y, float w, float h);
void fontPreRenderHW(unsigned char resetMatrix);

extern vbinfo* s_pDlVertexBuffers;
extern ibinfo* s_pDlIndexBuffers;

extern SCloudLayer_s s_cloudLayer;
extern PBuffer_s pbuffers[64];
extern unsigned __int8* s_wodge;
extern slinklistdef_s viewListAll;
extern float s_hdrLumAlpha;
extern float s_hdrLumWhiteSq;
extern float s_hdrMaxAdaptedLum;
extern float s_hdrMinAdaptedLum;
extern float s_hdrAdaptTimeScale;
extern float s_hdrBloomOffset;
extern float s_hdrBloomThreshold;
extern float s_hdrPowerLumCalc;
extern float s_hdrSampleRadius;
extern unsigned int s_hdrGaussian;
extern float s_hdrCompression;
extern unsigned int sFrameBuffer;
extern unsigned int sFrameBufferPrev;
extern unsigned int sTextureCircleSun;
extern int s_topIndex;
extern rStateBlock* s_top;

extern rStateValueStore* g_rState;

extern dlContextData idlContextData;
extern dlContextStruct idlContextStruct;