#include "hw.h"
#include "engine/engineinit.h"
#include "util/powerpc.h"
#include "engine/string/engstring.h"
#include "file/packfile.h"
#include "WindowsWindow.h"
#include "framework/template/CTemplate.h"

#include "engine/Font.h"

#include "engine/scene/scene.h"

#include "util/unorgtypes.h"

#include "dlvtx.h"

#include "util/x2tHelper.h"

#include "Logger/Log.h"

#include <iostream>
#include <vector>
#pragma comment(lib, "d3d9.lib")

IDirect3D9* g_pD3D = NULL;
IDirect3DDevice9* g_pd3dDevice = NULL;
IDirect3DTexture9* g_pD3dFrontBufferTex = NULL;

int s_initScreenWidth = -1;
int s_initScreenHeight = -1;
unsigned int s_bUseMultisampleTwo = 0u;
unsigned int s_bVerticalSync = 0u;
unsigned int s_texHwD3dFrontBuffer = 4294967295u;
IDirect3DSurface9* g_pD3dBackBufferTarget = NULL;
IDirect3DSurface9* g_pD3dDepthStencilTarget = NULL;
void* s_baseAddr = NULL;
windowdef_s windowfullscreen = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
rStateBlock s_toSend;

IDirect3DVertexShader9** s_pCustomVertexShaders = NULL;
IDirect3DPixelShader9** s_pCustomPixelShaders = NULL;
circularBuffer s_dlBuffer = { NULL, 0u, 0u, 0u };

unsigned __int8 s_bDlVBsInitialised = 0u;
vbinfo* s_pDlVertexBuffers = NULL;
ibinfo* s_pDlIndexBuffers;
IDirect3DVertexDeclaration9** s_pCustomVertexDecls = nullptr;
int s_dlHwMemGroup = 0;

int activeBuffer = 0;
debugdrawBuffer_s* currentBuffer = NULL;
unsigned __int8 s_bufferContentsAreValid = 1u;
debugdrawBuffer_s ddBuffer[1];
slinklistdef_s viewListAll = { NULL, 0 };

viewdef_s* s_currentview = NULL;

unsigned int sFrameBuffer = 4294967295u;
unsigned int sFrameBufferPrev = 4294967295u;

int sFrameSizeX = -1;
int sFrameSizeY = -1;
int s_multisample = 0;

unsigned int DL_TEXTURE_FRAME = 4294967295u;
unsigned int DL_TEXTURE_FRAME_SCRATCH = 4294967295u;
unsigned int DL_TEXTURE_FRAME_HALF = 4294967295u;
unsigned int DL_TEXTURE_FRAME_QUARTER = 4294967295u;
unsigned int DL_TEXTURE_FRAME_LAST = 4294967295u;
unsigned int DL_TEXTURE_DEPTH = 4294967295u;
unsigned int DL_PBUFFER_FRAME = 4294967295u;
unsigned int DL_PBUFFER_FRAME_SCRATCH = 4294967295u;
unsigned int DL_PBUFFER_FRAME_HALF = 4294967295u;
unsigned int DL_PBUFFER_FRAME_QUARTER = 4294967295u;
unsigned int DL_PBUFFER_FRAME_MULTISAMPLE = 4294967295u;

unsigned int DL_TEXTURE_ENV[6] = { 4294967295u, 4294967295u, 4294967295u, 4294967295u, 4294967295u, 4294967295u };
unsigned int s_envLightDownSampleTex = 4294967295u;

float s_hdrLumAlpha = 0.42500001;
float s_hdrLumWhiteSq = 1.5;
float s_hdrMaxAdaptedLum = 0.28999999;
float s_hdrMinAdaptedLum = 0.0099999998;
float s_hdrAdaptTimeScale = 90.0;
float s_hdrBloomOffset = 10.0;
float s_hdrBloomThreshold = 5.0;
float s_hdrPowerLumCalc = 1.0;
float s_hdrSampleRadius = 0.5;
unsigned int s_hdrGaussian = 4294967295u;
float s_hdrCompression = 2.0;

struct ScreenVertex
{
    float x, y, z, rhw;
    float u, v;
};

struct dlconstvtx_s
{
    PlatformColour_u col;
    vec4_u uv[4];
};

struct dlvars_s
{
    int                     bufferptr;
    dlconstvtx_s            constvtx;
    unsigned int            constusage;
    int                     clip_left;
    int                     clip_right;
    int                     clip_top;
    int                     clip_bottom;
    IDirect3DIndexBuffer9* indexbuffer;
    int                     indexptr;
    IDirect3DVertexBuffer9* vtxbuffer;
    int                     vtxptr;
    unsigned __int8* indexDatabuffer;
    unsigned __int8* indexDataptr;
    unsigned int            indexStartFrame;
    unsigned int            initalised;
    unsigned int            boundib;
    // PC additions for tracking pending draw call
    D3DPRIMITIVETYPE        pendingPrimType;
    int                     pendingNumVerts;
    unsigned int            pendingStride;
    int                     pendingVtxOffset;
};

#define SCREEN_FVF (D3DFVF_XYZRHW | D3DFVF_TEX1)

IDirect3DTexture9* g_TestGreenTexture = nullptr;

bool CreateTestGreenTexture(IDirect3DDevice9* device)
{
    if (!device)
        return false;

    if (g_TestGreenTexture)
        return true;

    HRESULT hr = device->CreateTexture(
        64,                     // width
        64,                     // height
        1,                      // mip levels
        0,                      // usage
        D3DFMT_A8R8G8B8,        // format
        D3DPOOL_MANAGED,        // pool
        &g_TestGreenTexture,
        NULL
    );

    if (FAILED(hr) || !g_TestGreenTexture)
        return false;

    D3DLOCKED_RECT locked = {};
    hr = g_TestGreenTexture->LockRect(0, &locked, NULL, 0);
    if (FAILED(hr))
        return false;

    for (int y = 0; y < 64; ++y)
    {
        unsigned int* row = (unsigned int*)((unsigned char*)locked.pBits + y * locked.Pitch);
        for (int x = 0; x < 64; ++x)
        {
            // A8R8G8B8 = 0xAARRGGBB
            row[x] = 0x8F01FF00; // solid green
        }
    }

    g_TestGreenTexture->UnlockRect(0);
    return true;
}

void DrawTestGreenQuad(IDirect3DDevice9* device, float x, float y, float width, float height)
{
    if (!device)
        return;

    if (!CreateTestGreenTexture(device))
        return;

    ScreenVertex verts[4];

    // -0.5f helps align texels/pixels properly in DX9 screen-space rendering
    float left = x - 0.5f;
    float top = y - 0.5f;
    float right = x + width - 0.5f;
    float bottom = y + height - 0.5f;

    verts[0] = { left,  top,    0.0f, 1.0f, 0.0f, 0.0f };
    verts[1] = { right, top,    0.0f, 1.0f, 1.0f, 0.0f };
    verts[2] = { left,  bottom, 0.0f, 1.0f, 0.0f, 1.0f };
    verts[3] = { right, bottom, 0.0f, 1.0f, 1.0f, 1.0f };

    device->SetTexture(0, g_TestGreenTexture);
    device->SetFVF(SCREEN_FVF);

    device->SetRenderState(D3DRS_LIGHTING, FALSE);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    device->SetRenderState(D3DRS_ZENABLE, FALSE);

    device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

    device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, verts, sizeof(ScreenVertex));
}

void DestroyTestGreenTexture()
{
    if (g_TestGreenTexture)
    {
        g_TestGreenTexture->Release();
        g_TestGreenTexture = nullptr;
    }
}

texdef_s textures[4096];
unsigned int s_blankTex = 4294967295u;

unsigned __int8* s_pTexturesHWUsed = NULL;
texdefHW* s_pTexturesHW = NULL;
int s_maxUsedTextureIndex = -1;

unsigned int texCreateHandle()
{
    unsigned int result = 0;
    texdef_s* tex = textures;

    // Find a free slot (flags & 1 == 0 means free)
    while (tex->flags & 1)
    {
        ++tex;
        ++result;
        if (result >= 0x1000)
            return (unsigned int)-1;
    }

    // Initialize the slot
    tex->id = -1;
    tex->streamflags = 1;
    tex->flags = 1;
    tex->count = 1;
    tex->width = -1;
    tex->height = -1;
    tex->depth = 1;
    tex->type = TEX_TYPE_UNKNOWN;

    // Initialize hw arrays
    for (int i = 0; i < 3; i++)
    {
        tex->hw[i] = (unsigned int)-1;
        tex->hwFrameNeeded[i] = 0;
        tex->hwFileSize[i] = 0;
    }

    tex->minLod = 0.0f;
    tex->nextframe = 0;
    tex->animinfo = nullptr;
    tex->lightmapScale = 0.0f;

    s_maxUsedTextureIndex = -1;

    return result;
}

unsigned int texCreateNullTexture(TexType type, int width, int height, unsigned int flags)
{
    unsigned int handle = texCreateHandle();
    if (handle == (unsigned int)-1)
        return (unsigned int)-1;

    texdef_s* tex = (handle < 0x1000) ? &textures[handle] : nullptr;
    if (!tex)
        return handle;

    tex->width = (short)width;
    tex->height = (short)height;
    tex->type = TEX_TYPE_2D;
    tex->flags = tex->flags | flags | 2;

    return handle;
}

void texFreeHWHandle(unsigned int texHwHandle)
{
    //texdefHW* hw = &s_pTexturesHW[texHwHandle];

    //// Unbind from any sampler slots that reference this handle
    //for (unsigned int i = 0; i < s_numSamplers; i++)
    //{
    //    if (s_samplers[i] == texHwHandle)
    //    {
    //        g_pd3dDevice->SetTexture(i, nullptr);
    //        s_samplers[i] = (unsigned int)-1;
    //    }
    //}

    //// Free video memory
    //if (hw->vmemHandle != (unsigned int)-1)
    //{
    //    s_vidMemFreeSpace += s_pVidMemContext->array[hw->vmemHandle].size;
    //    hmemFreeDelayed(s_pVidMemContext, hw->vmemHandle);
    //}

    //// Clear HW entry
    //hw->vmemHandle = (unsigned int)-1;
    //hw->texHandle = (unsigned int)-1;
    //hw->texLevel = -1;
    //hw->maxMipmapLevel = -1;

    //// Free the slot
    //s_pTexturesHWUsed[texHwHandle] = 0;

    //// Release D3D texture
    //if (hw->___u0.dxtex)
    //{
    //    hw->___u0.dxtex->Release();
    //    hw->___u0.dxtex = nullptr;
    //}
}
unsigned int s_samplers[16] = {};
unsigned int s_defaultInvalidTex;

D3DFORMAT xb360fmtToD3D(int fmtIndex)
{
    switch (fmtIndex)
    {
    case 0: return D3DFMT_DXT1;  // 0x18280086 - DXT1 no alpha
    case 1: return D3DFMT_DXT3;  // 0x1A200054 - DXT3
    case 2: return D3DFMT_DXT2;  // 0x1A200052 - DXT2
    case 3: return D3DFMT_DXT1;  // 0x18280186 - DXT1 with alpha
    case 4: return D3DFMT_DXT5;  // 0x1A200154 - DXT5
    case 5: return D3DFMT_DXT4;  // 0x1A200152 - DXT4
    default: return D3DFMT_A8R8G8B8;
    }
}

unsigned int texCreateTextureInplace(
    unsigned int th, unsigned int vmemh, int streamLevel,
    unsigned int sizeX, short sizeY, int sizeZ,
    int fmt, TexType texType, float lightmapScale, int flags)
{
    if (th >= 0x1000 || th == (unsigned int)-1)
        return th;

    texdef_s* tex = &textures[th];
    tex->lightmapScale = lightmapScale;
    tex->width = (short)sizeX;
    tex->height = sizeY;
    tex->type = texType;
    tex->flags = tex->flags | flags | 2;

    // Find free HW slot
    int hwSlot = -1;
    for (int i = 0; i < 0x1001; i++)
    {
        if (!s_pTexturesHWUsed[i])
        {
            s_pTexturesHWUsed[i] = 1;
            hwSlot = i;
            break;
        }
    }

    tex->hw[streamLevel] = hwSlot;

    texdefHW* hw = &s_pTexturesHW[hwSlot];
    hw->vmemHandle = vmemh;
    hw->texHandle = th;
    hw->texLevel = (short)streamLevel;
    hw->maxMipmapLevel = (short)sizeZ;
    hw->address = nullptr;

    // Map fmt to D3DFORMAT
    D3DFORMAT d3dfmt = xb360fmtToD3D(fmt);
    DbgPrint("xb360fmtToD3D(%d) = 0x%X", fmt, d3dfmt);
    DbgPrint("CreateTexture: %ux%u mips=%d fmt=0x%X", sizeX, sizeY, sizeZ + 1, d3dfmt);
    // Create D3D texture based on type
    HRESULT hr = E_FAIL;
    if (texType == TEX_TYPE_2D || texType == TEX_TYPE_HDR || texType == TEX_TYPE_RECTANGLE)
    {
        IDirect3DTexture9* d3dTex = nullptr;
        // Clamp mip levels to valid range for this texture size
        int maxMips = 1;
        {
            unsigned int s = (sizeX > (unsigned)sizeY) ? sizeX : (unsigned)sizeY;
            while (s > 1) { s >>= 1; maxMips++; }
        }
        int clampedMips = (sizeZ + 1 < maxMips) ? sizeZ + 1 : maxMips;
        DbgPrint("mips requested=%d clamped=%d maxValid=%d", sizeZ + 1, clampedMips, maxMips);

        hr = g_pd3dDevice->CreateTexture(
            sizeX, sizeY,
            clampedMips, 0,
            d3dfmt, D3DPOOL_MANAGED,
            &d3dTex, nullptr);
        DbgPrint("CreateTexture HRESULT: 0x%X", hr);

        if (SUCCEEDED(hr))
            hw->___u0.dxtex = d3dTex;

        if (FAILED(hr))
        {
            DbgPrint("CreateTexture FAILED: %ux%u mips=%d fmt=0x%X hr=0x%X",
                sizeX, sizeY, sizeZ + 1, (unsigned)d3dfmt, (unsigned)hr);
        }
    }
    else if (texType == TEX_TYPE_CUBE)
    {
        IDirect3DCubeTexture9* d3dCube = nullptr;
        hr = g_pd3dDevice->CreateCubeTexture(
            sizeX, sizeZ + 1, 0,
            d3dfmt, D3DPOOL_MANAGED,
            &d3dCube, nullptr);
        if (SUCCEEDED(hr))
            hw->___u0.dxcube = d3dCube;
    }
    else if (texType == TEX_TYPE_3D)
    {
        IDirect3DVolumeTexture9* d3dVol = nullptr;
        hr = g_pd3dDevice->CreateVolumeTexture(
            sizeX, sizeY, sizeZ, 1, 0,
            d3dfmt, D3DPOOL_MANAGED,
            &d3dVol, nullptr);
        if (SUCCEEDED(hr))
            hw->___u0.dxvol = d3dVol;
    }

    if (FAILED(hr))
    {
        s_pTexturesHWUsed[hwSlot] = 0;
        tex->hw[streamLevel] = (unsigned int)-1;
        return th;
    }

    // Free higher stream levels that are now superseded
    if (streamLevel + 1 < 3)
    {
        for (int i = streamLevel + 1; i < 3; i++)
        {
            if (tex->hw[i] != (unsigned int)-1)
            {
                texFreeHWHandle(tex->hw[i]);
                tex->hw[i] = (unsigned int)-1;
            }
        }
    }

    return th;
}

bool texLevelIsValid(unsigned int th, int level)
{
    if (th == (unsigned int)-1)
        return false;

    return textures[th].hw[level] != (unsigned int)-1;
}

unsigned int texGetTexID(unsigned int handle)
{
    if (handle == (unsigned int)-1)
        return (unsigned int)-1;

    texdef_s* tex = &textures[handle];

    // If animated, return the current frame's hw handle
    if (tex->animinfo)
        return textures[tex->animinfo->curframe].hw[0];

    // If not streaming or no file table entry or streaming flag set, return hw[0] directly
    if (!rdebugflags.texstream || tex->id == -1 || (tex->flags & 0x10000) != 0)
        return tex->hw[0];

    // Streaming path — find first valid mip level
    for (int level = 0; level < 3; level++)
    {
        if (texLevelIsValid(handle, level))
            return tex->hw[level];
    }

    return (unsigned int)-1;
}

struct SAssetType
{
    int assetType;
    const char* platformType;
    const char* platformExtension;
    int platformVersion;
};

SAssetType s_assetTypes[25] =
{
  { 0, "ob_xb", "/ob.rax", 184 },
  { 2, "ob_xb", ".rax", 184 },
  { 3, "nav_xb", ".rax", 5 },
  { 4, "atlas_xb", "/atlas.rax", 8 },
  { 6, "tex_xb", ".x2t", 7 },
  { 7, "mat_xb", ".rax", 1 },
  { 10, "anim_xb", "/anim.rax", 21 },
  { 5, "font_xb", ".xft", 9 },
  { 11, "dgeom_xb", "/dgeom.rax", 7 },
  { 12, "sound_xb", ".wav", 6 },
  { 13, "sound_xb", ".adpcm", 6 },
  { 14, "sound_xb", ".xma", 6 },
  { 15, "sm_xb", "/sm.hdr", 9 },
  { 16, "vistable", ".vt", 0 },
  { 17, "terrain_xb", "/terrain.ttr", 35 },
  { 18, "terrain_xb", "/foliage.raw", 35 },
  { 19, "animstream_xb", "/anim.rax", 5 },
  { 23, "animstreamcomp_xb", "/anim.rax", 2 },
  { 20, "mega_xb", ".mga", 11 },
  { 21, "cloud_xb", ".cld", 1 },
  { 22, "ob_xb", "/blk.rax", 184 },
  { 25, "embed_xb", ".war", 4 },
  { 26, "vmo", ".vmo", 2 },
  { 27, "omv", ".omv", 2 },
  { 24, "particle", ".res", 0 }
};

void writeTextureEx(int stage, int iHandle, int iFunc)
{
    if (iHandle & 0x80000000)
        return;

    // Resolve handle — fall back to blank tex if invalid
    unsigned int handle = iHandle;
    if (handle >= 0x1000 || handle == (unsigned int)-1)
        handle = s_blankTex;
    if (handle >= 0x1000 || handle == (unsigned int)-1)
    {
        g_pd3dDevice->SetTexture(stage, nullptr);
        s_samplers[stage] = (unsigned int)-1;
        return;
    }

    texdef_s* tex = &textures[handle];

    // Get HW texture ID
    int texID = texGetTexID(handle);
    if (texID == -1)
    {
        // Use default invalid tex if available
        if (s_defaultInvalidTex != (unsigned int)-1 && s_defaultInvalidTex < 0x1000)
        {
            texdefHW* hw = &s_pTexturesHW[textures[s_defaultInvalidTex].hw[0]];
            g_pd3dDevice->SetTexture(stage, hw->___u0.dxtex);
            s_samplers[stage] = textures[s_defaultInvalidTex].hw[0];
        }
        else
        {
            g_pd3dDevice->SetTexture(stage, nullptr);
            s_samplers[stage] = (unsigned int)-1;
        }
        return;
    }

    texdefHW* hw = &s_pTexturesHW[texID];
    s_samplers[stage] = texID;

    // Determine filter modes based on tex type and iFunc
    bool hasMips = (hw->maxMipmapLevel > 0);
    unsigned int flags = tex->flags;

    // Set the texture
    IDirect3DBaseTexture9* d3dTex = nullptr;
    switch (tex->type)
    {
    case TEX_TYPE_CUBE:
        d3dTex = hw->___u0.dxcube;
        break;
    case TEX_TYPE_3D:
        d3dTex = hw->___u0.dxvol;
        break;
    default:
        d3dTex = hw->___u0.dxtex;
        break;
    }
    g_pd3dDevice->SetTexture(stage, d3dTex);

    // Set filter states based on iFunc
    switch (iFunc)
    {
    case 1: case 2: case 3:
    {
        // Normal filtering
        bool noFilter = (flags & 0x10) != 0;
        g_pd3dDevice->SetSamplerState(stage, D3DSAMP_MINFILTER, noFilter ? D3DTEXF_NONE : D3DTEXF_LINEAR);
        g_pd3dDevice->SetSamplerState(stage, D3DSAMP_MAGFILTER, noFilter ? D3DTEXF_NONE : D3DTEXF_LINEAR);
        g_pd3dDevice->SetSamplerState(stage, D3DSAMP_MIPFILTER, hasMips ? D3DTEXF_LINEAR : D3DTEXF_NONE);
        break;
    }
    case 4:
    {
        // Point filtering
        g_pd3dDevice->SetSamplerState(stage, D3DSAMP_MINFILTER, D3DTEXF_POINT);
        g_pd3dDevice->SetSamplerState(stage, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
        g_pd3dDevice->SetSamplerState(stage, D3DSAMP_MIPFILTER, hasMips ? D3DTEXF_LINEAR : D3DTEXF_NONE);
        break;
    }
    case 5:
    {
        // Forced linear
        g_pd3dDevice->SetSamplerState(stage, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        g_pd3dDevice->SetSamplerState(stage, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        g_pd3dDevice->SetSamplerState(stage, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
        break;
    }
    case 6:
    {
        // No filtering
        g_pd3dDevice->SetSamplerState(stage, D3DSAMP_MINFILTER, D3DTEXF_NONE);
        g_pd3dDevice->SetSamplerState(stage, D3DSAMP_MAGFILTER, D3DTEXF_NONE);
        g_pd3dDevice->SetSamplerState(stage, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
        break;
    }
    default:
        break;
    }
}

TDirEntry s_dirPoolData[15000];
TDirEntry s_root = { NULL, NULL, NULL, NULL, 0u, "" };
poolState s_dirPool = { NULL, NULL, 0u, 0u, 0u, 0u };

TDirEntry* searchDir(TDirEntry* entry, const char* name, int length)
{
    TDirEntry* child = entry->children;
    if (!child)
        return nullptr;

    while (strncmp(child->name, name, length) || child->name[length] != 0)
    {
        child = child->next;
        if (!child)
            return nullptr;
    }

    return child;
}

int fileTableFindHandle(const char* fileName)
{
    TDirEntry* dir = &s_root;
    int len = 0;
    const char* seg = fileName;
    const char* p = fileName;

    if (*fileName)
    {
        do
        {
            char c = *p;
            if (c == '/' || c == '\\')
            {
                dir = searchDir(dir, seg, len);
                seg = p + 1;
                len = 0;
                if (!dir)
                    return -1;
            }
            else
            {
                len++;
            }
            p++;
        } while (*p);

        if (!dir)
            return -1;
    }

    TDirEntry* entry = searchDir(dir, seg, len);
    if (entry)
        return (int)((entry - s_dirPoolData) | _rotl(entry->rollingcount, 16));
    else
        return -1;
}

char* getName(char* buffer, TDirEntry* entry)
{
    char* v5;

    if (entry->parent == &s_root)
    {
        v5 = buffer;
    }
    else
    {
        char* name = getName(buffer, entry->parent);
        *name = '/';
        v5 = name + 1;
    }

    // Calculate length of entry->name (unrolled strlen, max 76 chars)
    unsigned int v6 = 0;
    int          v7 = 0;
    char* v8 = entry->name;

    while (v8[v7])
    {
        if (!entry->name[v7 + 1]) { v6 += 1; break; }
        if (!entry->name[v7 + 2]) { v6 += 2; break; }
        if (!entry->name[v7 + 3]) { v6 += 3; break; }
        v7 += 4;
        v6 += 4;
        if (v7 >= 76)
            break;
    }

    memcpy(v5, v8, v6);
    return v5 + v6;
}

int opencount = 0;
int fileStreamingClose(unsigned short streamhandle)
{
    unsigned int v1 = streamhandle;

    if (streamhandle > 0x12B)
        return 0;

    StreamInfo_s* v2 = &s_Streams[streamhandle];
    if (!v2)
        return 0;

    // Close OS handle if this was a single-open stream
    if (v2->opensingle)
        CloseHandle(v2->h);

    // Unlink from doubly linked list
    if (v2->prev)
        v2->prev->next = v2->next;
    if (v2->next)
        v2->next->prev = v2->prev;

    v2->next = nullptr;
    v2->prev = nullptr;

    if (s_CurStream == v2)
        s_CurStream = nullptr;

    // Reset stream slot
    if (v1 <= 0x12B)
    {
        StreamInfo_s* v7 = &s_Streams[v1];
        if (v7)
        {
            v7->handle = (v7->handle & 0xFFFF0000); // clear low word
            v7->h = (void*)-1;
            --opencount;
        }
    }

    return 1;
}

int fileTableGetFilename(char* buffer, unsigned int handle)
{
    if (handle != (unsigned int)-1)
    {
        unsigned int idx = (unsigned short)handle;
        if (s_dirPoolData[idx].rollingcount == (handle >> 16))
        {
            TDirEntry* entry = &s_dirPoolData[idx];
            if (entry)
            {
                char* end = getName(buffer, entry);
                *end = 0;
                return 1;
            }
        }
    }

    *buffer = 0;
    return 0;
}

void fileTableRemove(unsigned int handle)
{
    TDirEntry* v3 = nullptr;

    if (handle != (unsigned int)-1)
    {
        unsigned int idx = (unsigned short)handle;
        unsigned int rollingcount = s_dirPoolData[idx].rollingcount;
        if (rollingcount == (handle >> 16))
            v3 = &s_dirPoolData[idx];
    }

    while (v3)
    {
        TDirEntry* parent = v3->parent;
        if (!parent)
            break;

        // Unlink v3 from parent's children list
        TDirEntry* children = parent->children;
        if (children == v3)
        {
            parent->children = v3->next;
        }
        else
        {
            TDirEntry* i = children;
            while (i->next != v3)
                i = i->next;
            i->next = v3->next;
        }

        // Return v3 to free pool
        ++v3->rollingcount;
        v3->next = (TDirEntry*)s_dirPool.free;
        s_dirPool.free = (poolObject*)v3;
        ++s_dirPool.freeCount;

        v3 = parent;
        if (!parent->children)
            break;
    }
}

TDirEntry* addDirectory(TDirEntry* parent, const char* fileName, int length)
{
    TDirEntry* entry = (TDirEntry*)s_dirPool.free;
    if (!entry)
        return nullptr;

    s_dirPool.free = s_dirPool.free->next;
    s_dirPool.freeCount--;

    entry->children = nullptr;
    entry->next = parent->children;
    parent->children = entry;
    entry->parent = parent;

    int copyLen = (length >= 75) ? 74 : length;
    strncpy(entry->name, fileName, copyLen);
    entry->name[copyLen] = 0;

    return entry;
}

TDirEntry* addEntry(const char* fileName)
{
    TDirEntry* dir = &s_root;
    const char* seg = fileName;
    int len = 0;
    const char* p = fileName;

    // Walk path components, creating directories as needed
    while (*p)
    {
        char c = *p;
        if (c == '/' || c == '\\')
        {
            TDirEntry* found = searchDir(dir, seg, len);
            if (!found)
            {
                dir = addDirectory(dir, seg, len);
                seg = p + 1;
                break;
            }
            dir = found;
            seg = p + 1;
            len = 0;
        }
        else
        {
            len++;
        }
        p++;
    }

    // Walk remaining path components
    int flen = 0;
    const char* fseg = seg;
    const char* fp = seg;

    while (*fp)
    {
        char c = *fp;
        if (c == '/' || c == '\\')
        {
            TDirEntry* entry = (TDirEntry*)s_dirPool.free;
            if (entry)
            {
                s_dirPool.free = s_dirPool.free->next;
                s_dirPool.freeCount--;
                entry->children = nullptr;
                entry->next = dir->children;
                dir->children = entry;
                entry->parent = dir;
                int copyLen = (flen >= 75) ? 74 : flen;
                strncpy(entry->name, fseg, copyLen);
                entry->name[copyLen] = 0;
            }
            dir = entry;
            fseg = fp + 1;
            flen = 0;
        }
        else
        {
            flen++;
        }
        fp++;
    }

    // Add final entry (the filename itself)
    TDirEntry* entry = (TDirEntry*)s_dirPool.free;
    if (entry)
    {
        s_dirPool.free = s_dirPool.free->next;
        s_dirPool.freeCount--;
        entry->children = nullptr;
        entry->next = dir->children;
        dir->children = entry;
        entry->parent = dir;
        int copyLen = (flen >= 75) ? 74 : flen;
        strncpy(entry->name, fseg, copyLen);
        entry->name[copyLen] = 0;
    }

    return entry;
}

char* assetGetPathForType(int inAssetType, char* outPath, int inBuffSize)
{
    SAssetType* found = nullptr;
    for (SAssetType* a = s_assetTypes; (int)a < (int)&s_threadJobQueue; a++)
    {
        if (a->assetType == inAssetType)
        {
            found = a;
            break;
        }
    }

    if (!found || !g_settings)
    {
        *outPath = 0;
        return outPath;
    }

    char suffix[32];
    vafmtbuff(suffix, 32, "/%s_v%d/", found->platformType, found->platformVersion);

    const char* assetsdir = g_settings->assetsdir;
    //g_settings->GetStringValue("assetsdir", &assetsdir);

    vafmtbuff(outPath, inBuffSize, "%s%s", assetsdir ? assetsdir : "", suffix);

    return outPath;
}

void(__cdecl* s_assetRequestFunc)(const char*, int) = NULL;
unsigned int texLoadTextureName(const char* filename, int baseflags)
{
    if (!rdebugflags.enableTextures)
        return (unsigned int)-1;

    // Check if already loaded by file table handle
    if ((baseflags & 0x80) == 0)
    {
        int ftHandle = fileTableFindHandle(filename);
        if (ftHandle != -1)
        {
            for (unsigned int i = 0; i < 0x1000; i++)
            {
                if ((int)textures[i].ftHandle == ftHandle)
                {
                    textures[i].count++;
                    return i;
                }
            }
        }
    }

    // Build full path
    char fullpath[256] = {};
    if (initData.DisablePathExpansion)
    {
        vafmtbuff(fullpath, 256, "data/%s%s", "", filename);
    }
    else
    {
        char assetPath[256] = {};
        assetGetPathForType(6, assetPath, 256);
        vafmtbuff(fullpath, 256, "%s%s.x2t", assetPath, filename);
        if (s_assetRequestFunc)
            s_assetRequestFunc(filename, 6);
    }

    // Get file size and allocate memory
    unsigned int filesize = fileSize(fullpath);
    DbgPrint("x2t path: %s", fullpath);
    DbgPrint("x2t filesize: %u", filesize);
    if (filesize == 0)
        return (unsigned int)-1;

    unsigned char* buf = (unsigned char*)malloc(filesize);
    if (!buf)
        return (unsigned int)-1;

    int fh = fileOpen(fullpath, 1);
    if (fh == -1)
    {
        free(buf);
        return (unsigned int)-1;
    }
    DbgPrint("Reading file: %s filesize=%u", fullpath, filesize);
    DbgPrint("buf address: %p", buf);

    unsigned int bytesRead = fileRead(fh, buf, filesize);

    DbgPrint("Immediately after fileRead: buf[0..7]: %02X %02X %02X %02X %02X %02X %02X %02X",
        buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
    fileClose(fh);
    DbgPrint("After fileClose: buf[0..7]: %02X %02X %02X %02X %02X %02X %02X %02X",
        buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);

    if (bytesRead != filesize)
    {
        free(buf);
        return (unsigned int)-1;
    }

    DbgPrint("buf[0..7]:      %02X %02X %02X %02X %02X %02X %02X %02X", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
    DbgPrint("buf[4096..4103]:%02X %02X %02X %02X %02X %02X %02X %02X", buf[4096], buf[4097], buf[4098], buf[4099], buf[4100], buf[4101], buf[4102], buf[4103]);
    DbgPrint("buf[16384..16391]:%02X %02X %02X %02X %02X %02X %02X %02X", buf[16384], buf[16385], buf[16386], buf[16387], buf[16388], buf[16389], buf[16390], buf[16391]);

    DbgPrint("buf[filesize-8216..filesize-8209]: %02X %02X %02X %02X %02X %02X %02X %02X",
        buf[filesize - 8216], buf[filesize - 8215], buf[filesize - 8214], buf[filesize - 8213],
        buf[filesize - 8212], buf[filesize - 8211], buf[filesize - 8210], buf[filesize - 8209]);

    // Find first non-zero byte
    for (int i = 0; i < (int)filesize - 24; i++) {
        if (buf[i] != 0) {
            DbgPrint("First non-zero byte at offset %d: 0x%02X", i, buf[i]);
            break;
        }
    }
    // Find last non-zero byte  
    for (int i = (int)filesize - 25; i >= 0; i--) {
        if (buf[i] != 0) {
            DbgPrint("Last non-zero byte at offset %d: 0x%02X", i, buf[i]);
            break;
        }
    }

    // DEBUG
    DbgPrint("bytesRead: %u filesize: %u", bytesRead, filesize);
    DbgPrint("Last 24 bytes of buf:");
    unsigned char* dbgEnd = buf + filesize;
    for (int i = 24; i > 0; i--)
        DbgPrint("  end[-%d] = 0x%02X", i, *(dbgEnd - i));

    // Parse header with correct big-endian reads
    X2THeader hdr = x2tParseHeader(buf + filesize);

    uint32_t sizeX = hdr.sizeX;
    uint32_t sizeY = hdr.sizeY;
    uint16_t sizeZ = hdr.mipsStored;
    uint32_t fmt = hdr.fmt;

    // flags2 and lightmap logic still uses raw end pointer for fields
    // not covered by X2THeader (flags2 is covered — use hdr.flags2)
    int flags = baseflags;
    if (hdr.numMips == 1)
        flags |= 0x40;

    float lightmapScale = 1.0f;
    TexType texType = TEX_TYPE_2D;

    if (hdr.flags2 & 1)
    {
        // Lightmap texture — bpp field tells us which flag to set
        int shift = (hdr.bpp == 32) ? 0x1000000 : 0x800000;
        flags |= shift;
        lightmapScale = 1.0f;
        texType = TEX_TYPE_2D;
    }
    else
    {
        // mipCount field overlaps with lmScale in original code
        lightmapScale = (float)hdr.mipCount * 0.1f;
        texType = TEX_TYPE_2D;
    }

    // Create texture handle
    unsigned int handle = texCreateHandle();
    if (handle == (unsigned int)-1)
    {
        free(buf);
        return (unsigned int)-1;
    }

    // Create D3D texture with correct BE-parsed dimensions and format
    unsigned int result = texCreateTextureInplace(
        handle, (unsigned int)(uintptr_t)buf, 0,
        sizeX, (short)sizeY, sizeZ,
        fmt, texType, lightmapScale, flags);

    if (result != (unsigned int)-1)
    {
        texdefHW* hw = &s_pTexturesHW[textures[result].hw[0]];
        DbgPrint("hw[0]=%u dxtex=%p", textures[result].hw[0], hw->___u0.dxtex);

        if (hw->___u0.dxtex)
        {
            DbgPrint("Calling x2tUploadToD3D: fmt=%u sizeX=%u sizeY=%u numMips=%u",
                hdr.fmt, hdr.sizeX, hdr.sizeY,
                hdr.numMips > 0 ? hdr.numMips : (uint32_t)(sizeZ + 1));

            bool uploadOk = x2tUploadToD3D(
                buf, filesize,
                hw->___u0.dxtex,
                hdr.sizeX, hdr.sizeY,
                hdr.fmt,
                hdr.mipsStored > 0 ? hdr.mipsStored : 1);

            DbgPrint("x2tUploadToD3D returned: %d", uploadOk);
        }
        else
        {
            DbgPrint("dxtex is null, skipping upload");
        }
    }

    if (result == (unsigned int)-1)
    {
        free(buf);
        return (unsigned int)-1;
    }

    // Detile and upload pixel data
    texdefHW* hw = &s_pTexturesHW[textures[result].hw[0]];
    if (hw->___u0.dxtex)
    {
        x2tUploadToD3D(
            buf, filesize,
            hw->___u0.dxtex,
            hdr.sizeX, hdr.sizeY,
            hdr.fmt,
            hdr.numMips > 0 ? hdr.numMips : (uint32_t)(sizeZ + 1));
    }

    textures[result].flags &= ~2u;

    if ((flags & 0x80) == 0)
    {
        void* entry = addEntry(filename);
        textures[result].ftHandle = (unsigned int)(
            ((unsigned char*)entry - (unsigned char*)s_dirPoolData) / 96);
    }

    return result;
}

void texFreeTexture(unsigned int handle)
{
    if (handle == (unsigned int)-1)
        return;

    texdef_s* tex = &textures[handle];

    unsigned short newCount = (unsigned short)(tex->count - 1);
    tex->count = newCount;

    if (newCount != 0)
        return;

    // Free all 3 HW levels
    for (unsigned int i = 0; i < 3; i++)
        //texDeleteTextureLevelHW(handle, i);

    // If not a null texture and no file table entry, remove from file table
    if ((tex->flags & 2) == 0 && tex->id < 0)
    {
        //fileTableRemove(tex->ftHandle);
        //tex->ftHandle = (unsigned int)-1;
    }

    tex->flags = 0;
}

void texGenBlankTexture()
{
    unsigned int handle = texCreateNullTexture(TEX_TYPE_2D, 64, 64, 0);
    s_blankTex = handle;

    if (handle == (unsigned int)-1)
        return;

    texdef_s* tex = (handle < 0x1000) ? &textures[handle] : nullptr;
    if (!tex)
        return;

    // Find a free HW slot
    int hwSlot = -1;
    for (int i = 0; i < 0x1001; i++)
    {
        if (!s_pTexturesHWUsed[i])
        {
            s_pTexturesHWUsed[i] = 1;
            hwSlot = i;
            break;
        }
    }

    if (hwSlot == -1)
        return;

    tex->hw[0] = hwSlot;

    // Create D3D texture
    IDirect3DTexture9* d3dTex = nullptr;
    HRESULT hr = g_pd3dDevice->CreateTexture(
        tex->width, tex->height,
        1, 0,
        D3DFMT_A8R8G8B8,
        D3DPOOL_MANAGED,
        &d3dTex, nullptr);

    if (FAILED(hr))
    {
        tex->hw[0] = -1;
        texFreeTexture(s_blankTex);
        s_blankTex = -1;
        return;
    }

    // Fill with 0x7F7F7F7F
    D3DLOCKED_RECT locked;
    if (SUCCEEDED(d3dTex->LockRect(0, &locked, nullptr, 0)))
    {
        unsigned int* pixels = (unsigned int*)locked.pBits;
        for (int i = 0; i < 64 * 64; i++)
            pixels[i] = 0x7F7F7F7F;
        d3dTex->UnlockRect(0);
    }

    // Store in HW table using correct struct fields
    texdefHW* hw = &s_pTexturesHW[hwSlot];
    hw->___u0.dxtex = d3dTex;
    hw->vmemHandle = (unsigned int)-1;
    hw->address = nullptr;
    hw->texHandle = s_blankTex;
    hw->texLevel = 0;
    hw->maxMipmapLevel = 0;

    tex->hwFileSize[0] = 64 * 64 * 4;
}

unsigned int texCreateRenderTexture(int width, int height, unsigned int flags)
{
    unsigned int handle = texCreateNullTexture(TEX_TYPE_2D, width, height, flags | 0x400);
    if (handle == (unsigned int)-1)
        return (unsigned int)-1;

    texdef_s* tex = &textures[handle];
    tex->flags |= 0x400;
    tex->lightmapScale = 1.0f;

    // Determine D3D format based on flags
    D3DFORMAT fmt;
    if (flags & 0x8)
        fmt = D3DFMT_D24S8;
    else if (flags & 0x800)
    {
        tex->type = TEX_TYPE_HDR;
        if (flags & 0x1000)
            fmt = D3DFMT_A16B16G16R16F;
        else
            fmt = D3DFMT_A8R8G8B8;
    }
    else if (flags & 0x2000000)
        fmt = D3DFMT_A8R8G8B8;
    else
        fmt = D3DFMT_A8R8G8B8;

    // Find free HW slot
    int hwSlot = -1;
    for (int i = 0; i < 0x1001; i++)
    {
        if (!s_pTexturesHWUsed[i])
        {
            s_pTexturesHWUsed[i] = 1;
            hwSlot = i;
            break;
        }
    }

    if (hwSlot == -1)
    {
        texFreeTexture(handle);
        return (unsigned int)-1;
    }

    tex->hw[0] = hwSlot;

    int alignedWidth = (width + 7) & ~7;
    int alignedHeight = (height + 7) & ~7;

    // Determine usage — depth/shadow needs D3DUSAGE_DEPTHSTENCIL, others RENDERTARGET
    DWORD usage = (flags & 0x8) ? D3DUSAGE_DEPTHSTENCIL : D3DUSAGE_RENDERTARGET;

    IDirect3DTexture9* d3dTex = nullptr;
    HRESULT hr = g_pd3dDevice->CreateTexture(
        alignedWidth, alignedHeight,
        1, usage,
        fmt,
        D3DPOOL_DEFAULT,
        &d3dTex, nullptr);

    if (FAILED(hr))
    {
        texFreeHWHandle(hwSlot);
        tex->hw[0] = (unsigned int)-1;
        texFreeTexture(handle);
        return (unsigned int)-1;
    }

    texdefHW* hw = &s_pTexturesHW[hwSlot];
    hw->___u0.dxtex = d3dTex;
    hw->vmemHandle = (unsigned int)-1;
    hw->address = nullptr;
    hw->texHandle = handle;
    hw->texLevel = 0;
    hw->maxMipmapLevel = 0;

    tex->hwFileSize[0] = alignedWidth * alignedHeight * 4;

    return handle;
}

void d3dCreateBackBufferRenderTarget(
    unsigned int width,
    unsigned int height,
    D3DFORMAT backBufferFormat,
    D3DFORMAT depthStencilFormat,
    D3DMULTISAMPLE_TYPE multiSampleType) {

    HRESULT hr;

    // Release existing surfaces if they exist
    if (g_pD3dBackBufferTarget) {
        g_pD3dBackBufferTarget->Release();
        g_pD3dBackBufferTarget = nullptr;
    }
    if (g_pD3dDepthStencilTarget) {
        g_pD3dDepthStencilTarget->Release();
        g_pD3dDepthStencilTarget = nullptr;
    }

    // Create the back buffer render target
    hr = g_pd3dDevice->CreateRenderTarget(
        width,
        height,
        backBufferFormat,
        multiSampleType,
        0,             // Multisample quality (0 for default)
        TRUE,          // Lockable (TRUE or FALSE depending on needs)
        &g_pD3dBackBufferTarget,
        nullptr
    );

    if (FAILED(hr)) {
        MessageBox(nullptr, L"Failed to create back buffer render target", L"Error", MB_OK | MB_ICONERROR);
        //TERMINATE FUNCTION
        //hwTerminate();
        return;
    }

    // Create the depth stencil target
    hr = g_pd3dDevice->CreateDepthStencilSurface(
        width,
        height,
        depthStencilFormat,
        multiSampleType,
        0,             // Multisample quality
        TRUE,          // Discard (TRUE to enable z-buffer discard)
        &g_pD3dDepthStencilTarget,
        nullptr
    );

    if (FAILED(hr)) {
        MessageBox(nullptr, L"Failed to create depth stencil surface", L"Error", MB_OK | MB_ICONERROR);
        //TERMINATE FUNCTION
        //hwTerminate();
        return;
    }
}

void d3dCreateFrontBufferTexture(unsigned int width, unsigned int height, D3DFORMAT frontBufferFormat) {
    HRESULT hr;

    // Release the existing front buffer texture if it exists
    if (g_pD3dFrontBufferTex) {
        g_pD3dFrontBufferTex->Release();
        g_pD3dFrontBufferTex = nullptr;
    }

    // Create the front buffer texture with specified width, height, and format
    hr = g_pd3dDevice->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, frontBufferFormat, D3DPOOL_DEFAULT, &g_pD3dFrontBufferTex, nullptr);
    if (FAILED(hr)) {
        // Handle the error (for example, by logging or throwing an exception)
        MessageBox(nullptr, L"Failed to create front buffer texture", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    // Assuming texture tracking array: mark this texture as used
    s_texHwD3dFrontBuffer = 1;  // Mark texture usage or set a unique ID if needed
}

void InitD3D() {
    D3DPRESENT_PARAMETERS d3dpp;
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Setting initial screen width and height based on the resolution and debug flags
    if (rdebugflags.enableHighDef) {
        s_initScreenWidth = min(s_initScreenWidth, screenWidth);
        s_initScreenHeight = min(s_initScreenHeight, screenHeight);
    }
    else {
        s_initScreenWidth = screenWidth;
        s_initScreenHeight = screenHeight;
    }

    // Initialize Direct3D
    g_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (g_pD3D == nullptr) {
        MessageBox(NULL, L"Failed to create Direct3D interface object", NULL, NULL);
        //TERMINATE FUNCTION
        //hwTerminate();
        return;
    }

    // Zero out the presentation parameters structure
    ZeroMemory(&d3dpp, sizeof(d3dpp));

    // Set presentation parameters
    //d3dpp.BackBufferWidth = s_initScreenWidth;
    //d3dpp.BackBufferHeight = s_initScreenHeight;
    d3dpp.BackBufferWidth = 1920;
    d3dpp.BackBufferHeight = 1080;

    d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    d3dpp.BackBufferCount = 1;
    d3dpp.MultiSampleType = s_bUseMultisampleTwo ? D3DMULTISAMPLE_2_SAMPLES : D3DMULTISAMPLE_NONE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    if (WinWindows::GameWindow->m_hAppWindow)
        std::cout << "window->m_hAppWindow" << std::endl;
    d3dpp.hDeviceWindow = WinWindows::GameWindow->m_hAppWindow; //GetForegroundWindow();
    d3dpp.Windowed = TRUE; // Fullscreen
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
    d3dpp.PresentationInterval = s_bVerticalSync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;

    // Create the Direct3D device
    //D3DDEVTYPE_HAL, d3dpp.hDeviceWindow

    if (!WinWindows::GameWindow->m_hAppWindow)
    {
        MessageBox(NULL, L"Window Null", NULL, NULL);
    }

    HRESULT hr = g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, WinWindows::GameWindow->m_hAppWindow,
        D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &g_pd3dDevice);
    if (FAILED(hr)) {
        //TERMINATE FUNCTION
        MessageBox(NULL, L"Unable to create DX Device", NULL, NULL);
        //hwTerminate();
        return;
    }



    // Create front buffer texture and back buffer render target
    d3dCreateFrontBufferTexture(d3dpp.BackBufferWidth, d3dpp.BackBufferHeight, D3DFMT_A8R8G8B8);
    d3dCreateBackBufferRenderTarget(d3dpp.BackBufferWidth, d3dpp.BackBufferHeight, d3dpp.BackBufferFormat,
        d3dpp.AutoDepthStencilFormat, d3dpp.MultiSampleType);

    // Set render targets
    LPDIRECT3DSURFACE9 pBackBuffer = nullptr;
    g_pd3dDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
    g_pd3dDevice->SetRenderTarget(0, pBackBuffer);
    pBackBuffer->Release();

    LPDIRECT3DSURFACE9 pDepthStencil = nullptr;
    g_pd3dDevice->GetDepthStencilSurface(&pDepthStencil);
    g_pd3dDevice->SetDepthStencilSurface(pDepthStencil);
    pDepthStencil->Release();

    // Finalize screen setup
    s_initScreenWidth = d3dpp.BackBufferWidth;
    s_initScreenHeight = d3dpp.BackBufferHeight;

    //TOODO 
    //Figure OUT
    //windowSetFullScreen(d3dpp.BackBufferWidth, d3dpp.BackBufferHeight);

    // Set default sampler states
    g_pd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    g_pd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
}

rStateBlock s_stack[16];
rStateValueStore* g_rState = &s_stack[0].values;

rStateBlock* s_top = &s_stack[0];
PlatformColour_u clearcolour;

PlatformColour_u platformColour(float r, float g, float b, float a)
{
    // Clamp all channels to [0, 1]
    if (r <= 0.0f) r = 0.0f;
    else if (r >= 1.0f) r = 1.0f;

    if (g <= 0.0f) g = 0.0f;
    else if (g >= 1.0f) g = 1.0f;

    if (b <= 0.0f) b = 0.0f;
    else if (b >= 1.0f) b = 1.0f;

    if (a <= 0.0f) a = 0.0f;
    else if (a >= 1.0f) a = 1.0f;

    PlatformColour_u result;
    result.__s0.a = (uint8_t)(int)(a * 255.0f);
    result.__s0.r = (uint8_t)(int)(r * 255.0f);
    result.__s0.g = (uint8_t)(int)(g * 255.0f);
    result.__s0.b = (uint8_t)(int)(b * 255.0f);
    return result;
}

void writeClearColour(float r, float g, float b, float a)
{
    DbgPrint("writeClearColour: r=%.2f g=%.2f b=%.2f a=%.2f", r, g, b, a);

    clearcolour.col = platformColour(r, g, b, a).col;

    DbgPrint("clearcolour.col = 0x%08X", clearcolour.col);

}

void setState(rStateBase* base, int offset, unsigned int numValues)
{
    // Get pointer to the slot in s_toSend.values at the given offset
    char* slot = (char*)&s_toSend.values + offset;
    slinkdef_s* link = (slinkdef_s*)slot;

    // Link into s_toSend changed list if not already linked
    if (link->next == link)
    {
        link->next = s_toSend.changed.head;
        s_toSend.changed.head = link;
    }

    // Copy writeCB, vtype, numValues from base into slot
    rStateBase* dst = (rStateBase*)slot;
    dst->writeCB.writeInt1 = base->writeCB.writeInt1;
    dst->vtype = base->vtype;
    dst->numValues = (uint8_t)numValues;

    // Copy the value data words from base into slot
    uint32_t* src = (uint32_t*)((char*)base + 0xC);
    uint32_t* dst2 = (uint32_t*)((char*)slot + 0xC);
    for (unsigned int i = 0; i < numValues; i++)
        dst2[i] = src[i];
}

int s_topIndex = 0;
void platformClearColourHW(float r, float g, float b, float a)
{
    DbgPrint("g_rState = %p", g_rState);
    DbgPrint("&s_stack[0] = %p", &s_stack[0]);
    DbgPrint("s_top = %p", s_top);
    DbgPrint("s_topIndex = %d", s_topIndex);
    DbgPrint("platformClearColourHW: r=%.2f g=%.2f b=%.2f a=%.2f", r, g, b, a);

    rStateValueStore* v4 = g_rState;
    rStateFloat4* p_clearcolour = &g_rState->clearcolour;

    p_clearcolour->f[0] = r;
    p_clearcolour->f[1] = g;
    p_clearcolour->f[2] = b;
    p_clearcolour->f[3] = a;
    p_clearcolour->base.writeCB.writeFloat4 = writeClearColour;

    if (p_clearcolour->base.link.next == (slinkdef_s*)p_clearcolour)
    {
        p_clearcolour->base.link.next = s_top->changed.head;
        s_top->changed.head = &p_clearcolour->base.link;
    }

    p_clearcolour->base.vtype = 0x400;
    p_clearcolour->base.numValues = 4;

    setState(&p_clearcolour->base,
        (char*)p_clearcolour - (char*)v4,
        4);
}

void BeginScene()
{
    g_pd3dDevice->BeginScene();
}

void EndScene()
{
    g_pd3dDevice->EndScene();
}

void RenderTest()
{
    g_pd3dDevice->BeginScene();
    platformClearBufferHW(1);

    DbgPrint("Before dlPush2D");
    dlPush2D();
    DbgPrint("After dlPush2D");

    platformColourMaskHW(1, 1, 1, 1);
    platformBlendHW(0);

    DbgPrint("Before dlFillRectangleZ");
    dlFillRectangleZ(0.1f, 0.1f, 0.9f, 0.9f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
    DbgPrint("After dlFillRectangleZ");

    rStatePop();

    g_pd3dDevice->EndScene();
    platformSwapBuffersHW();
}

int s_texHwMemGroup = 0;

unsigned int __fastcall texHwInit()
{
    int v0; // r10
    unsigned int* p_texHandle; // r11

    s_pTexturesHWUsed = (unsigned __int8*)memAllocAlignCore(
        0x1001u,
        s_texHwMemGroup,
        0,
        "source/platform_xenon/texHW.c",
        125,
        0,
        1);
    memset(s_pTexturesHWUsed, 0, 0x1001u);
    s_pTexturesHW = (texdefHW*)memAllocAlignCore(
        0x44044u,
        s_texHwMemGroup,
        0,
        "source/platform_xenon/texHW.c",
        130,
        0,
        1);
    memset(s_pTexturesHW, 0, 0x44044u);
    v0 = 4097;
    p_texHandle = &s_pTexturesHW->texHandle;
    do
    {
        --v0;
        *(p_texHandle - 1) = -1;
        *p_texHandle = -1;
        *((WORD*)p_texHandle + 2) = -1;
        *((WORD*)p_texHandle + 3) = -1;
        p_texHandle += 17;
    } while (v0);
    return 1;
}

ShaderUVSetInfo uvtestinfo_static[18] =
{
  {
    { { 'd', 'i', 'f', 'f', '\0', '\0', '\0', '\0' } },
    0,
    1u,
    SHUVFLG_NONE,
    NULL,
    0u
  },
  { { "bump" }, 0, 1u, SHUVFLG_NONE, NULL, 0u },
  { { "spec" }, 0, 1u, SHUVFLG_NONE, NULL, 0u },
  { { "incan" }, 0, 1u, SHUVFLG_NONE, NULL, 0u },
  { { "height" }, 0, 1u, SHUVFLG_NONE, NULL, 0u },
  { { "tranpcy" }, 1, 2u, SHUVFLG_NONE, NULL, 0u },
  { { "blend" }, 1, 2u, SHUVFLG_NONE, NULL, 0u },
  { { "diff2" }, 2, 4u, SHUVFLG_NONE, NULL, 0u },
  { { "bump2" }, 2, 4u, SHUVFLG_NONE, NULL, 0u },
  { { "spec2" }, 2, 4u, SHUVFLG_NONE, NULL, 0u },
  { { "envmap" }, 3, 8u, SHUVFLG_NONE, NULL, 0u },
  { { "lmap" }, 3, 8u, SHUVFLG_NONE, NULL, 0u },
  { { "occl" }, 3, 8u, SHUVFLG_NONE, NULL, 0u },
  { { "wdiff2" }, 1, 2u, SHUVFLG_NONE, NULL, 0u },
  { { "wbump2" }, 1, 2u, SHUVFLG_NONE, NULL, 0u },
  { { "wdiff3" }, 2, 4u, SHUVFLG_NONE, NULL, 0u },
  { { "wbump3" }, 2, 4u, SHUVFLG_NONE, NULL, 0u },
  { { "" }, 0, 0u, SHUVFLG_NONE, NULL, 0u }
};

SCloudLayer_s s_cloudLayer;
float cleardepth = 1.0;
void writeClearDepth(float depth)
{
    cleardepth = depth;
}

void writeDepthTest(int value)
{
    if (value == 1)
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, 1);
    else if (value == 0)
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, 0);
}

void platformClearDepthHW(float depth)
{
    rStateValueStore* v1 = g_rState;
    rStateFloat1* p_cleardepth = &g_rState->cleardepth;

    p_cleardepth->f[0] = 1.0f;
    p_cleardepth->base.writeCB.writeFloat1 = writeClearDepth;

    if (p_cleardepth->base.link.next == (slinkdef_s*)p_cleardepth)
    {
        p_cleardepth->base.link.next = s_top->changed.head;
        s_top->changed.head = &p_cleardepth->base.link;
    }

    p_cleardepth->base.vtype = 0x100;
    p_cleardepth->base.numValues = 1;

    setState(&p_cleardepth->base,
        (char*)p_cleardepth - (char*)v1,
        1);
}

void platformDepthTestHW(int value)
{
    rStateValueStore* v1 = g_rState;
    rStateInt1* p_depthtest = &g_rState->depthtest;

    p_depthtest->i[0] = value;
    p_depthtest->base.writeCB.writeInt1 = writeDepthTest;

    if (p_depthtest->base.link.next == (slinkdef_s*)p_depthtest)
    {
        p_depthtest->base.link.next = s_top->changed.head;
        s_top->changed.head = &p_depthtest->base.link;
    }

    p_depthtest->base.vtype = 1;
    p_depthtest->base.numValues = 1;

    setState(&p_depthtest->base,
        (char*)p_depthtest - (char*)v1,
        1);
}

void writeDepthFunc(int value)
{
    DWORD d3dFunc = 0;
    switch (value)
    {
    case 20: d3dFunc = 1; break; // D3DCMP_NEVER
    case 21: d3dFunc = 3; break; // D3DCMP_LESSEQUAL
    case 22: d3dFunc = 4; break; // D3DCMP_GREATEREQUAL
    case 23: d3dFunc = 6; break; // D3DCMP_NOTEQUAL
    case 24: d3dFunc = 2; break; // D3DCMP_LESS
    case 25: d3dFunc = 5; break; // D3DCMP_GREATER
    case 26: d3dFunc = 7; break; // D3DCMP_ALWAYS
    default: return;
    }
    g_pd3dDevice->SetRenderState(D3DRS_ZFUNC, d3dFunc);
}

void platformDepthFuncHW(int value)
{
    rStateValueStore* v1 = g_rState;
    rStateInt1* p_depthfunc = &g_rState->depthfunc;

    p_depthfunc->i[0] = value;
    p_depthfunc->base.writeCB.writeInt1 = writeDepthFunc;

    if (p_depthfunc->base.link.next == (slinkdef_s*)p_depthfunc)
    {
        p_depthfunc->base.link.next = s_top->changed.head;
        s_top->changed.head = &p_depthfunc->base.link;
    }

    p_depthfunc->base.vtype = 1;
    p_depthfunc->base.numValues = 1;

    setState(&p_depthfunc->base,
        (char*)p_depthfunc - (char*)v1,
        1);
}

void writeDepthWrite(int value)
{
    g_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, value != 0 ? 1 : 0);
}

void platformDepthWriteHW(int value)
{
    rStateValueStore* v1 = g_rState;
    rStateInt1* p_depthwrite = &g_rState->depthwrite;

    p_depthwrite->i[0] = value;
    p_depthwrite->base.writeCB.writeInt1 = writeDepthWrite;

    if (p_depthwrite->base.link.next == (slinkdef_s*)p_depthwrite)
    {
        p_depthwrite->base.link.next = s_top->changed.head;
        s_top->changed.head = &p_depthwrite->base.link;
    }

    p_depthwrite->base.vtype = 1;
    p_depthwrite->base.numValues = 1;

    setState(&p_depthwrite->base,
        (char*)p_depthwrite - (char*)v1,
        1);
}

void writeFaceCull(int value)
{
    DWORD cullMode;

    if ((value & 1) == 0)
    {
        cullMode = D3DCULL_NONE; // 1
    }
    else
    {
        cullMode = D3DCULL_CCW; // 3, was 6 on Xbox
        if (value == 3)
            cullMode = D3DCULL_CW; // 2, same on both
    }

    g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, cullMode);
}

void writeDepthBias(float factor, float units)
{
    // __real_39800000 = 0.00024414062f (1/4096)
    const float scale = 0.00024414062f;

    float scaledFactor = factor * scale;
    float scaledUnits = units * scale;

    g_pd3dDevice->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS,
        *(DWORD*)&scaledFactor);
    g_pd3dDevice->SetRenderState(D3DRS_DEPTHBIAS,
        *(DWORD*)&scaledUnits);
}

void platformDepthBias(float factor, float units)
{
    rStateValueStore* v2 = g_rState;
    rStateFloat2* p_depthbias = &g_rState->depthbias;

    p_depthbias->f[0] = factor;
    p_depthbias->f[1] = units;
    p_depthbias->base.writeCB.writeFloat2 = writeDepthBias;

    if (p_depthbias->base.link.next == (slinkdef_s*)p_depthbias)
    {
        p_depthbias->base.link.next = s_top->changed.head;
        s_top->changed.head = &p_depthbias->base.link;
    }

    p_depthbias->base.vtype = 0x200;
    p_depthbias->base.numValues = 2;

    setState(&p_depthbias->base,
        (char*)p_depthbias - (char*)v2,
        2);
}

void writeWireframe(int value)
{
    if (value == 1)
        g_pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME); // 2 on PC
    else
        g_pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);     // 3 on PC
}

void platformWireframeHW(int value)
{
    rStateValueStore* v1 = g_rState;
    rStateInt1* p_wireframe = &g_rState->wireframe;

    p_wireframe->i[0] = value;
    p_wireframe->base.writeCB.writeInt1 = writeWireframe;

    if (p_wireframe->base.link.next == (slinkdef_s*)p_wireframe)
    {
        p_wireframe->base.link.next = s_top->changed.head;
        s_top->changed.head = &p_wireframe->base.link;
    }

    p_wireframe->base.vtype = 1;
    p_wireframe->base.numValues = 1;

    setState(&p_wireframe->base,
        (char*)p_wireframe - (char*)v1,
        1);
}

/*
TODO CONFIRM THIS IS LIKE RIGHT?
*/
static DWORD xboxBlendToPC(DWORD xb)
{
    // Xbox 360 D3DBLEND is PC D3DBLEND + 1, except 0 = ZERO (same as PC 1)
    switch (xb)
    {
    case 0:  return D3DBLEND_ZERO;         // 1
    case 1:  return D3DBLEND_ONE;          // 2
    case 4:  return D3DBLEND_SRCALPHA;     // 5
    case 5:  return D3DBLEND_INVSRCALPHA;  // 6
    case 6:  return D3DBLEND_SRCALPHA;     // 5  (standard alpha src)
    case 7:  return D3DBLEND_INVSRCALPHA;  // 6  (standard alpha dst)
    case 8:  return D3DBLEND_DESTCOLOR;    // 9
    case 10: return D3DBLEND_SRCCOLOR;     // 3
    case 11: return D3DBLEND_INVSRCCOLOR;  // 4
    default: return D3DBLEND_ONE;
    }
}

void writeBlend(int value)
{
    if (value == 0)
    {
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
        return;
    }

    DWORD src = 0, dst = 0;
    switch (value)
    {
    case 2:
    case 3:  src = 6;  dst = 7;  break;
    case 4:
    case 18: src = 6;  dst = 1;  break;
    case 5:
    case 16: src = 0;  dst = 7;  break;
    case 7:  src = 4;  dst = 0;  break;
    case 8:  src = 8;  dst = 0;  break;
    case 9:  src = 8;  dst = 1;  break;
    case 10: src = 1;  dst = 1;  break;
    case 11: src = 4;  dst = 5;  break;
    case 12: src = 10; dst = 1;  break;
    case 13: src = 11; dst = 1;  break;
    case 14: src = 10; dst = 0;  break;
    case 15: src = 0;  dst = 6;  break;
    case 17: src = 10; dst = 11; break;
    case 19: src = 1;  dst = 7;  break;
    default: return;
    }

    g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, xboxBlendToPC(src));
    g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, xboxBlendToPC(dst));
    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
}

void platformBlendHW(int value)
{
    rStateValueStore* v1 = g_rState;
    rStateInt1* p_blend = &g_rState->blend;

    p_blend->i[0] = value;
    p_blend->base.writeCB.writeInt1 = writeBlend;

    if (p_blend->base.link.next == (slinkdef_s*)p_blend)
    {
        p_blend->base.link.next = s_top->changed.head;
        s_top->changed.head = &p_blend->base.link;
    }

    p_blend->base.vtype = 1;
    p_blend->base.numValues = 1;

    setState(&p_blend->base,
        (char*)p_blend - (char*)v1,
        1);
}

void writeScissorTest(int value)
{
    if (value == 1)
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, 1);
    else if (value == 0)
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, 0);
    // value > 1: early return, do nothing
}

void dlScissorTest(unsigned char enable)
{
    rStateValueStore* v1 = g_rState;
    rStateInt1* p_scissortest = &g_rState->scissortest;

    // cntlzw/extrwi/xori sequence = (enable != 0) ? 1 : 0
    p_scissortest->i[0] = (enable != 0) ? 1 : 0;
    p_scissortest->base.writeCB.writeInt1 = writeScissorTest;

    if (p_scissortest->base.link.next == (slinkdef_s*)p_scissortest)
    {
        p_scissortest->base.link.next = s_top->changed.head;
        s_top->changed.head = &p_scissortest->base.link;
    }

    p_scissortest->base.vtype = 1;
    p_scissortest->base.numValues = 1;

    setState(&p_scissortest->base,
        (char*)p_scissortest - (char*)v1,
        1);
}

void writeScissorGL(int x, int y, int width, int height)
{
    RECT rect;
    rect.left = x;
    rect.top = y;
    rect.right = x + width;
    rect.bottom = y + height;
    g_pd3dDevice->SetScissorRect(&rect);
}

void platformScissorGL(int x, int y, int width, int height)
{
    rStateValueStore* v4 = g_rState;
    rStateInt4* p_scissor = &g_rState->scissor;

    p_scissor->i[0] = x;
    p_scissor->i[1] = y;
    p_scissor->i[2] = width;
    p_scissor->i[3] = height;
    p_scissor->base.writeCB.writeInt4 = writeScissorGL;

    if (p_scissor->base.link.next == (slinkdef_s*)p_scissor)
    {
        p_scissor->base.link.next = s_top->changed.head;
        s_top->changed.head = &p_scissor->base.link;
    }

    p_scissor->base.vtype = 4;
    p_scissor->base.numValues = 4;

    setState(&p_scissor->base,
        (char*)p_scissor - (char*)v4,
        4);
}

void writeViewportGL(int x, int y, int width, int height)
{
    D3DVIEWPORT9 vp;
    vp.X = x;
    vp.Y = y;
    vp.Width = width;
    vp.Height = height;
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;
    g_pd3dDevice->SetViewport(&vp);
}

void platformViewportGL(int x, int y, int width, int height)
{
    rStateValueStore* v4 = g_rState;
    rStateInt4* p_viewport = &g_rState->viewport;

    p_viewport->i[0] = x;
    p_viewport->i[1] = y;
    p_viewport->i[2] = width;
    p_viewport->i[3] = height;
    p_viewport->base.writeCB.writeInt4 = writeViewportGL;

    if (p_viewport->base.link.next == (slinkdef_s*)p_viewport)
    {
        p_viewport->base.link.next = s_top->changed.head;
        s_top->changed.head = &p_viewport->base.link;
    }

    p_viewport->base.vtype = 4;
    p_viewport->base.numValues = 4;

    setState(&p_viewport->base,
        (char*)p_viewport - (char*)v4,
        4);
}

void writePointSize(float size)
{
    // fctidz converts float to integer (truncate) then stores as DWORD
    DWORD isize = (DWORD)(int)size;
    g_pd3dDevice->SetRenderState(D3DRS_POINTSIZE, isize);
}

void dlSetPointSize(float size)
{
    rStateValueStore* v1 = g_rState;
    rStateFloat1* p_pointsize = &g_rState->pointsize;

    p_pointsize->f[0] = size;
    p_pointsize->base.writeCB.writeFloat1 = writePointSize;

    if (p_pointsize->base.link.next == (slinkdef_s*)p_pointsize)
    {
        p_pointsize->base.link.next = s_top->changed.head;
        s_top->changed.head = &p_pointsize->base.link;
    }

    p_pointsize->base.vtype = 0x100;
    p_pointsize->base.numValues = 1;

    setState(&p_pointsize->base,
        (char*)p_pointsize - (char*)v1,
        1);
}

dlvars_s s_dl;

void writeLineWidth(float width)
{
    //Not needed for pc probably
    return;
}

void dlSetLineWidth(float width)
{
    rStateValueStore* v1 = g_rState;
    rStateFloat1* p_linewidth = &g_rState->linewidth;

    p_linewidth->f[0] = width;
    p_linewidth->base.writeCB.writeFloat1 = writeLineWidth;

    if (p_linewidth->base.link.next == (slinkdef_s*)p_linewidth)
    {
        p_linewidth->base.link.next = s_top->changed.head;
        s_top->changed.head = &p_linewidth->base.link;
    }

    p_linewidth->base.vtype = 0x100;
    p_linewidth->base.numValues = 1;

    setState(&p_linewidth->base,
        (char*)p_linewidth - (char*)v1,
        1);
}

void writeAlphaTest(int test, int ref)
{
    if (test == 0)
    {
        g_pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, 0);
        return;
    }

    g_pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, 1);

    if (test == 40)
    {
        g_pd3dDevice->SetRenderState(D3DRS_ALPHAFUNC, 6); // D3DCMP_GREATEREQUAL
        g_pd3dDevice->SetRenderState(D3DRS_ALPHAREF, ref);
    }
    else if (test == 41)
    {
        g_pd3dDevice->SetRenderState(D3DRS_ALPHAFUNC, 4); // D3DCMP_EQUAL
        g_pd3dDevice->SetRenderState(D3DRS_ALPHAREF, ref);
    }
    // other test values: alpha test enabled but func/ref not set
}

void platformAlphaTestHW(int test, int ref)
{
    rStateValueStore* v2 = g_rState;
    rStateInt2* p_alphatest = &g_rState->alphatest;

    p_alphatest->i[0] = test;
    p_alphatest->i[1] = ref;
    p_alphatest->base.writeCB.writeInt2 = writeAlphaTest;

    if (p_alphatest->base.link.next == (slinkdef_s*)p_alphatest)
    {
        p_alphatest->base.link.next = s_top->changed.head;
        s_top->changed.head = &p_alphatest->base.link;
    }

    p_alphatest->base.vtype = 2;
    p_alphatest->base.numValues = 2;

    setState(&p_alphatest->base,
        (char*)p_alphatest - (char*)v2,
        2);
}

void writeShade(int value)
{
    // Xbox 360 shade mode enum -> PC D3DSHADEMODE
    // 28 (0x1C) is the only value ever passed (hardcoded in platformShadeHW)
    // D3DSHADEMODE: D3DSHADE_FLAT=1, D3DSHADE_GOURAUD=2, D3DSHADE_PHONG=3
    switch (value)
    {
    case 28: // likely GOURAUD (smooth shading)
        g_pd3dDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
        break;
    case 29: // likely FLAT
        g_pd3dDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT);
        break;
    default:
        g_pd3dDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
        break;
    }
}

void platformShadeHW(int value)
{
    rStateValueStore* v1 = g_rState;
    rStateInt1* p_shade = &g_rState->shade;

    p_shade->i[0] = 28; // hardcoded, value param is ignored
    p_shade->base.writeCB.writeInt1 = writeShade;

    if (p_shade->base.link.next == (slinkdef_s*)p_shade)
    {
        p_shade->base.link.next = s_top->changed.head;
        s_top->changed.head = &p_shade->base.link;
    }

    p_shade->base.vtype = 1;
    p_shade->base.numValues = 1;

    setState(&p_shade->base,
        (char*)p_shade - (char*)v1,
        1);
}

void writeStencilTest(int value)
{
    if (value == 1)
        g_pd3dDevice->SetRenderState(D3DRS_STENCILENABLE, 1);
    else if (value == 0)
        g_pd3dDevice->SetRenderState(D3DRS_STENCILENABLE, 0);
    // value > 1: early return, do nothing
}

void platformStencilTestHW(int value)
{
    rStateValueStore* v1 = g_rState;
    rStateInt1* p_stenciltest = &g_rState->stenciltest;

    p_stenciltest->i[0] = value;
    p_stenciltest->base.writeCB.writeInt1 = writeStencilTest;

    if (p_stenciltest->base.link.next == (slinkdef_s*)p_stenciltest)
    {
        p_stenciltest->base.link.next = s_top->changed.head;
        s_top->changed.head = &p_stenciltest->base.link;
    }

    p_stenciltest->base.vtype = 1;
    p_stenciltest->base.numValues = 1;

    setState(&p_stenciltest->base,
        (char*)p_stenciltest - (char*)v1,
        1);
}

void writeStencilFunc(int func, int ref, int mask)
{
    // Map game stencil func enum (29-35) to D3DCMPFUNC (1-7)
    DWORD d3dFunc = 0;
    switch (func)
    {
    case 29: d3dFunc = 7; break; // D3DCMP_ALWAYS
    case 30: d3dFunc = 2; break; // D3DCMP_LESS
    case 31: d3dFunc = 5; break; // D3DCMP_GREATER
    case 32: d3dFunc = 1; break; // D3DCMP_NEVER
    case 33: d3dFunc = 3; break; // D3DCMP_LESSEQUAL
    case 34: d3dFunc = 4; break; // D3DCMP_GREATEREQUAL
    case 35: d3dFunc = 6; break; // D3DCMP_NOTEQUAL
    default: break;
    }

    if (d3dFunc != 0)
        g_pd3dDevice->SetRenderState(D3DRS_STENCILFUNC, d3dFunc);

    g_pd3dDevice->SetRenderState(D3DRS_STENCILREF, (DWORD)ref);
    g_pd3dDevice->SetRenderState(D3DRS_STENCILMASK, (DWORD)mask);
}

void platformStencilFuncHW(int func, int ref, int mask)
{
    rStateValueStore* v3 = g_rState;
    rStateInt3* p_stencilfunc = &g_rState->stencilfunc;

    p_stencilfunc->i[0] = func;
    p_stencilfunc->i[1] = ref;
    p_stencilfunc->i[2] = mask;
    p_stencilfunc->base.writeCB.writeInt3 = writeStencilFunc;

    if (p_stencilfunc->base.link.next == (slinkdef_s*)p_stencilfunc)
    {
        p_stencilfunc->base.link.next = s_top->changed.head;
        s_top->changed.head = &p_stencilfunc->base.link;
    }

    p_stencilfunc->base.vtype = 3;
    p_stencilfunc->base.numValues = 3;

    setState(&p_stencilfunc->base,
        (char*)p_stencilfunc - (char*)v3,
        3);
}

static DWORD stencilOpToD3D(int op)
{
    switch (op)
    {
    case 36: return 0; // D3DSTENCILOP_KEEP? -- 0 is actually invalid, likely D3DSTENCILOP_KEEP=1
    case 37: return 6; // D3DSTENCILOP_INVERT
    case 38: return 7; // D3DSTENCILOP_INCRSAT or similar
    case 39: return 2; // D3DSTENCILOP_ZERO
    default: return 0;
    }
}

void writeStencilOp(int fail, int zfail, int pass)
{
    DWORD d3dFail = stencilOpToD3D(fail);
    if (fail >= 36 && fail <= 39)
        g_pd3dDevice->SetRenderState(D3DRS_STENCILFAIL, d3dFail);

    DWORD d3dZFail = stencilOpToD3D(zfail);
    if (zfail >= 36 && zfail <= 39)
        g_pd3dDevice->SetRenderState(D3DRS_STENCILZFAIL, d3dZFail);

    DWORD d3dPass = stencilOpToD3D(pass);
    if (pass >= 36 && pass <= 39)
        g_pd3dDevice->SetRenderState(D3DRS_STENCILPASS, d3dPass);
}

void platformStencilOpHW(int fail, int zfail, int pass)
{
    rStateValueStore* v3 = g_rState;
    rStateInt3* p_stencilop = &g_rState->stencilop;

    p_stencilop->i[0] = fail;
    p_stencilop->i[1] = zfail;
    p_stencilop->i[2] = pass;
    p_stencilop->base.writeCB.writeInt3 = writeStencilOp;

    if (p_stencilop->base.link.next == (slinkdef_s*)p_stencilop)
    {
        p_stencilop->base.link.next = s_top->changed.head;
        s_top->changed.head = &p_stencilop->base.link;
    }

    p_stencilop->base.vtype = 3;
    p_stencilop->base.numValues = 3;

    setState(&p_stencilop->base,
        (char*)p_stencilop - (char*)v3,
        3);
}

void writeColourMask(int r, int g, int b, int a)
{
    DWORD mask = 0;
    if (r == 1) mask |= D3DCOLORWRITEENABLE_RED;   // 1
    if (g == 1) mask |= D3DCOLORWRITEENABLE_GREEN;  // 2
    if (b == 1) mask |= D3DCOLORWRITEENABLE_BLUE;   // 4
    if (a == 1) mask |= D3DCOLORWRITEENABLE_ALPHA;  // 8

    g_pd3dDevice->SetRenderState(D3DRS_COLORWRITEENABLE, mask);
}

void platformColourMaskHW(int r, int g, int b, int a)
{
    rStateValueStore* v4 = g_rState;
    rStateInt4* p_colourmask = &g_rState->colourmask;

    p_colourmask->i[0] = r;
    p_colourmask->i[1] = g;
    p_colourmask->i[2] = b;
    p_colourmask->i[3] = a;
    p_colourmask->base.writeCB.writeInt4 = writeColourMask;

    if (p_colourmask->base.link.next == (slinkdef_s*)p_colourmask)
    {
        p_colourmask->base.link.next = s_top->changed.head;
        s_top->changed.head = &p_colourmask->base.link;
    }

    p_colourmask->base.vtype = 4;
    p_colourmask->base.numValues = 4;

    setState(&p_colourmask->base,
        (char*)p_colourmask - (char*)v4,
        4);
}

void texSelectTextureEx(int handle, TexFunc func)
{
    rStateValueStore* v2 = g_rState;
    rStateInt3* texture = g_rState->texture;

    texture->i[0] = 0;
    texture->i[1] = handle;
    texture->i[2] = 0;
    texture->base.writeCB.writeInt3 = writeTextureEx;

    if (texture->base.link.next == (slinkdef_s*)texture)
    {
        texture->base.link.next = s_top->changed.head;
        s_top->changed.head = &texture->base.link;
    }

    texture->base.vtype = 3;
    texture->base.numValues = 3;

    setState(&texture->base,
        (char*)texture - (char*)v2,
        3);
}

unsigned __int8 g_shaderEnabledVP = 0u;
unsigned __int8 g_shaderEnabledFP = 0u;
void dlSetDefaultState()
{
    platformClearColourHW(0.0f, 0.0f, 0.0f, 1.0f);
    platformClearDepthHW(1.0f);
    platformDepthTestHW(1);
    platformDepthFuncHW(21);
    platformDepthWriteHW(1);

    // Set face cull render state
    rStateValueStore* v1 = g_rState;
    v1->facecull.i[0] = 3;
    v1->facecull.base.writeCB.writeInt1 = writeFaceCull;

    if (v1->facecull.base.link.next == (slinkdef_s*)v1)
    {
        v1->facecull.base.link.next = s_top->changed.head;
        s_top->changed.head = &v1->facecull.base.link;
    }

    v1->facecull.base.vtype = 1;
    v1->facecull.base.numValues = 1;
    setState(&v1->facecull.base, 0, 1);

    platformDepthBias(0.0f, 0.0f);
    platformWireframeHW(0);
    platformBlendHW(0);
    dlScissorTest(0);

    platformScissorGL(0, 0, (int)windowfullscreen.width, (int)windowfullscreen.height);
    platformViewportGL(0, 0, (int)windowfullscreen.width, (int)windowfullscreen.height);

    dlSetPointSize(1.0f);
    dlSetLineWidth(1.0f);

    s_dl.boundib = 0;
    g_pd3dDevice->SetIndices(nullptr);

    platformAlphaTestHW(0, 0);
    platformShadeHW(28);
    platformStencilTestHW(0);
    platformStencilFuncHW(29, 0, 0);
    platformStencilOpHW(36, 36, 36);
    platformColourMaskHW(1, 1, 1, 0);

    texSelectTextureEx(-1, TEX_FUNC_MODULATE_RGBA);

    g_shaderEnabledVP = 0;
    g_shaderEnabledFP = 0;
}

void sendState(rStateBaseEx* cached, const rStateBaseEx* a2)
{
    unsigned int vtype = a2->base.vtype;

    if (vtype == 0x100)
    {
        // 1 float
        cached->args[0].f = a2->args[0].f;
        a2->base.writeCB.writeFloat1(a2->args[0].f);
    }
    else if (vtype > 0x100)
    {
        switch (vtype)
        {
        case 0x200:
            // 2 floats
            cached->args[0].f = a2->args[0].f;
            cached->args[1].f = a2->args[1].f;
            a2->base.writeCB.writeFloat2(a2->args[0].f, a2->args[1].f);
            break;
        case 0x300:
            // 3 floats
            cached->args[0].f = a2->args[0].f;
            cached->args[1].f = a2->args[1].f;
            cached->args[2].f = a2->args[2].f;
            a2->base.writeCB.writeFloat3(a2->args[0].f, a2->args[1].f, a2->args[2].f);
            break;
        case 0x400:
            // 4 floats
            cached->args[0].f = a2->args[0].f;
            cached->args[1].f = a2->args[1].f;
            cached->args[2].f = a2->args[2].f;
            cached->args[3].f = a2->args[3].f;
            a2->base.writeCB.writeFloat4(a2->args[0].f, a2->args[1].f, a2->args[2].f, a2->args[3].f);
            break;
        default:
            return;
        }
    }
    else
    {
        switch (vtype)
        {
        case 1:
            cached->args[0].i = a2->args[0].i;
            a2->base.writeCB.writeInt1(a2->args[0].i);
            break;
        case 2:
            cached->args[0].i = a2->args[0].i;
            cached->args[1].i = a2->args[1].i;
            a2->base.writeCB.writeInt2(a2->args[0].i, a2->args[1].i);
            break;
        case 3:
            cached->args[0].i = a2->args[0].i;
            cached->args[1].i = a2->args[1].i;
            cached->args[2].i = a2->args[2].i;
            a2->base.writeCB.writeInt3(a2->args[0].i, a2->args[1].i, a2->args[2].i);
            break;
        case 4:
            cached->args[0].i = a2->args[0].i;
            cached->args[1].i = a2->args[1].i;
            cached->args[2].i = a2->args[2].i;
            cached->args[3].i = a2->args[3].i;
            a2->base.writeCB.writeInt4(a2->args[0].i, a2->args[1].i, a2->args[2].i, a2->args[3].i);
            break;
        case 5:
            cached->args[0].i = a2->args[0].i;
            cached->args[1].i = a2->args[1].i;
            cached->args[2].i = a2->args[2].i;
            cached->args[3].i = a2->args[3].i;
            cached->args[4].i = a2->args[4].i;
            a2->base.writeCB.writeInt5(a2->args[0].i, a2->args[1].i, a2->args[2].i, a2->args[3].i, a2->args[4].i);
            break;
        case 6:
            cached->args[0].i = a2->args[0].i;
            cached->args[1].i = a2->args[1].i;
            cached->args[2].i = a2->args[2].i;
            cached->args[3].i = a2->args[3].i;
            cached->args[4].i = a2->args[4].i;
            cached->args[5].i = a2->args[5].i;
            a2->base.writeCB.writeInt6(a2->args[0].i, a2->args[1].i, a2->args[2].i, a2->args[3].i, a2->args[4].i, a2->args[5].i);
            break;
        default:
            return;
        }
    }
}

unsigned __int8 occqueryused[512];
ShaderTreeIteration* s_shaders_0[2];
unsigned __int8 occqueryenabled = 0u;
IDirect3DQuery9* occqueries[512];

void occQueryInitialise()
{
    // Clear the used flags array
    memset(occqueryused, 0, 0x200);

    // Create occlusion queries for each slot up to pak
    IDirect3DQuery9** v1 = occqueries;
    while ((int)v1 < (int)&pak)
    {
        if (v1)
        {
            g_pd3dDevice->CreateQuery(D3DQUERYTYPE_OCCLUSION, v1);
        }
        ++v1;
    }

    occqueryenabled = 1;
}


void platformStateInitialiseHW()
{
    DbgPrint("platformStateInitialiseHW called, s_topIndex=%d", s_topIndex);

    // Initialize s_toSend - set changed list to point to itself (empty)
    s_toSend.changed.offset = 0;
    s_toSend.changed.head = (slinkdef_s*)&s_toSend;

    // Initialize all value slots in s_toSend to self-referencing links
    char* p = (char*)&s_toSend.values;
    for (int i = 0; i < 229; i++, p += 4)
        *(slinkdef_s**)p = (slinkdef_s*)p;

    // Initialize all rStateBlock entries in s_stack up to s_cloudLayer
    rStateBlock* block = s_stack;
    while ((int)block < (int)&s_cloudLayer)
    {
        block->changed.offset = 0;
        block->changed.head = (slinkdef_s*)block;

        char* v = (char*)&block->values;
        for (int i = 0; i < 229; i++, v += 4)
            *(slinkdef_s**)v = (slinkdef_s*)v;

        ++block;
    }

    dlSetDefaultState();

    rStateWrite(); // flush all default states including clearcolour


    // Reset s_toSend changed list
    s_toSend.changed.head = (slinkdef_s*)&s_toSend;

    DbgPrint("Before push: s_topIndex=%d", s_topIndex);
    // Push a new render state layer if stack isn't full
    if (s_topIndex + 1 < 16)
    {
        DbgPrint("Pushing state layer");
        ++s_topIndex;
        ++s_top;
        g_rState = &s_top->values;

    }
    else
    {
        DbgPrint("Push skipped - stack full");
    }

    // PC-specific: set half pixel offset render state
    //g_pd3dDevice->SetRenderState(D3DRS_HALFPIXELOFFSET, 1);

    occQueryInitialise();
}

mtx_u mviewmtxcur;
mtx_u perspmtxcur;
stackheader_s mviewmtxstack;
stackheader_s perspmtxstack;
mtx_u g_identMtx;
mtx_u platformmvp;
mtx_u platformmv;
unsigned __int8 platformmvitDirty = 1u;

void matrixMultiplyAligned(mtx_u* m, const mtx_u* m1, const mtx_u* m2)
{
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            m->f[i][j] = m1->f[i][0] * m2->f[0][j]
                + m1->f[i][1] * m2->f[1][j]
                + m1->f[i][2] * m2->f[2][j]
                + m1->f[i][3] * m2->f[3][j];
        }
    }
}

void UpdateMVP()
{
    matrixMultiplyAligned(&platformmvp, &mviewmtxcur, &perspmtxcur);

    // Copy mviewmtxcur into platformmv
    memcpy(&platformmv, &mviewmtxcur, sizeof(mtx_u));

    platformmvitDirty = 1;
}

unsigned int texHwMemCopyOnGpuInit()
{
    // Xbox 360 only: uses Xenos GPU memory export feature
    // (writing shader outputs directly to memory via g_xvs_memexport)
    // No equivalent on PC D3D9 - skip entirely
    return 0;
}

rStateValueStore s_sent;
void rStateWrite()
{
    slinkdef_s* head = s_toSend.changed.head;

    if ((rStateBlock*)head != &s_toSend)
    {
        do
        {
            // Get the new state slot
            const rStateBaseEx* newState = (const rStateBaseEx*)((char*)head + s_toSend.changed.offset);

            // Get cached state slot (offset into s_sent)
            char* toSendBase = (char*)&s_toSend.values + 8;
            int slotOffset = (char*)newState - toSendBase;
            rStateBaseEx* cachedState = (rStateBaseEx*)((char*)&s_sent + slotOffset);

            unsigned int numValues = newState->base.numValues;

            if (numValues > 0)
            {
                // Check if any value differs from cached
                int dirty = 0;
                for (unsigned int i = 0; i < numValues; i++)
                    dirty |= (newState->args[i].i - cachedState->args[i].i);

                if (dirty)
                    sendState(cachedState, newState);
            }

            // Unlink from changed list
            rStateBlock* next = (rStateBlock*)head->next;
            head->next = head; // self-link (remove from list)
            head = (slinkdef_s*)next;

        } while ((rStateBlock*)head != &s_toSend);
    }

    // Reset changed list to empty (self-referencing)
    s_toSend.changed.head = (slinkdef_s*)&s_toSend;
}

dlContextData idlContextData = { 0, 0, 0, 0, NULL };
dlContextStruct idlContextStruct = { &idlContextData, NULL };

float* dlGetBuffer(int type, int numVertices, int primitiveType, int constcol)
{
    unsigned int sizeInFloats = dlVtxInfo[type].sizeInFloats;

    // Check shaders are available
   /* if (!g_shaderEnabledVP && !s_pCustomVertexShaders[type])
        return nullptr;
    if (!g_shaderEnabledFP && !s_pCustomPixelShaders[type])
        return nullptr;*/

    rStateWrite();

    // Set vertex declaration
    //g_pd3dDevice->SetVertexDeclaration(s_pCustomVertexDecls[type]);

    // Set vertex shader and upload MVP matrix + const color
    if (!g_shaderEnabledVP)
    {
        //shaderSetVS(s_pCustomVertexShaders[type]);

        // Upload MVP matrix to VS constant register 0
        g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&platformmvp, 4);

        // Upload const color to VS constant register 4
        // Convert from 0-255 bytes to 0-1 floats
        const float inv255 = 1.0f / 255.0f;
        float col[4];
        col[0] = s_dl.constvtx.col.__s0.r * inv255;
        col[1] = s_dl.constvtx.col.__s0.g * inv255;
        col[2] = s_dl.constvtx.col.__s0.b * inv255;
        col[3] = s_dl.constvtx.col.__s0.a * inv255;
        g_pd3dDevice->SetVertexShaderConstantF(4, col, 1);
    }

    //if (!g_shaderEnabledFP)
        //shaderSetPS(s_pCustomPixelShaders[type]);

    // Map game primitive type to D3D primitive type


    D3DPRIMITIVETYPE d3dPrim;

    switch (primitiveType)
    {
    case 0:  d3dPrim = D3DPT_POINTLIST;     break;
    case 1:  d3dPrim = D3DPT_LINELIST;      break;
    case 2:  d3dPrim = D3DPT_LINESTRIP;     break;
    case 3:  d3dPrim = D3DPT_TRIANGLELIST;  break;
    case 4:  d3dPrim = D3DPT_TRIANGLESTRIP; break;
    case 5:
    case 7:  d3dPrim = D3DPT_TRIANGLEFAN;   break;
    case 6:
    case 8:
    case 9:
    case 10: d3dPrim = (D3DPRIMITIVETYPE)13; break; // Xbox quad list - no PC equivalent
    default: return nullptr;
    }

    //pbufferPreRenderNotify(1, 1);

    // On Xbox 360: D3DDevice_BeginVertices returns a pointer to locked vertex buffer memory
    // On PC: lock the vertex buffer manually
    UINT stride = sizeInFloats * sizeof(float);
    UINT size = numVertices * stride;

    void* pData = nullptr;
    if (FAILED(s_dl.vtxbuffer->Lock(s_dl.vtxptr * stride, size, &pData, D3DLOCK_NOOVERWRITE)))
        return nullptr;

    // Store draw call info for dlEndMain
    s_dl.pendingPrimType = d3dPrim;
    s_dl.pendingNumVerts = numVertices;
    s_dl.pendingStride = stride;
    s_dl.pendingVtxOffset = s_dl.vtxptr;

    return (float*)pData;
}

float* dlBeginMain(dlContextStruct* dl, int primitiveType, int numvertices, int type)
{
    DbgPrint("dlBeginMain: initalised=%d", s_dl.initalised);
    DbgPrint("dlBeginMain: dl=%p dl->data=%p &idlContextData=%p", dl, dl->data, &idlContextData);
    if (!s_dl.initalised)
    {
        DbgPrint("dlBeginMain: FAIL not initialised");
        return nullptr;
    }

    int v5 = numvertices;

    // primitiveType 8 = indexed quads -- adjust vertex count
    if (primitiveType == 8)
        v5 = (4 * numvertices) | (numvertices & 0x40000000);

    dlContextData* data = dl->data;

    DbgPrint("dlBeginMain: data=%p match=%d", data, data == &idlContextData);

    float* result = nullptr;
    if (data == &idlContextData)
    {
        DbgPrint("dlBeginMain: calling dlGetBuffer type=%d verts=%d prim=%d", type, v5 & ~0x40000000, primitiveType);
        result = dlGetBuffer(type, v5 & ~0x40000000, primitiveType, type);
        DbgPrint("dlBeginMain: dlGetBuffer returned %p", result);
    }
    else
    {
        DbgPrint("dlBeginMain: FAIL data mismatch");
        result = nullptr;
    }

    data->startPtr = result;

    if (result)
    {
        data->primitiveType = primitiveType;
        data->numvertexes = v5;
        data->type = type;
        data->vertexSize = dlVtxInfo[type].sizeInFloats;
    }

    return result;
}

void dlEndMain(dlContextData* dlData, float* dlPtr)
{
    int vertexSize = dlData->vertexSize;
    DbgPrint("dlEndMain: vertexSize=%d startPtr=%p dlPtr=%p", vertexSize, dlData->startPtr, dlPtr);

    if (vertexSize <= 0)
    {
        DbgPrint("dlEndMain: FAIL vertexSize <= 0");
        return;
    }

    int bytesWritten = (char*)dlPtr - (char*)dlData->startPtr;
    int numVertices = (bytesWritten / sizeof(float)) / vertexSize;

    DbgPrint("dlEndMain: bytesWritten=%d numVertices=%d", bytesWritten, numVertices);
    DbgPrint("dlEndMain: shaderVP=%d shaderFP=%d vsShader=%p psShader=%p",
        g_shaderEnabledVP, g_shaderEnabledFP,
        s_pCustomVertexShaders[dlData->type],
        s_pCustomPixelShaders[dlData->type]);

    if (numVertices > 0
        && (g_shaderEnabledVP || s_pCustomVertexShaders[dlData->type])
        && (g_shaderEnabledFP || s_pCustomPixelShaders[dlData->type]))
    {
        DbgPrint("dlEndMain: unlocking and drawing primType=%d stride=%d offset=%d",
            s_dl.pendingPrimType, s_dl.pendingStride, s_dl.pendingVtxOffset);

        s_dl.vtxbuffer->Unlock();
        g_pd3dDevice->SetStreamSource(0, s_dl.vtxbuffer,
            s_dl.pendingVtxOffset * s_dl.pendingStride,
            s_dl.pendingStride);
        HRESULT hr = g_pd3dDevice->DrawPrimitive(s_dl.pendingPrimType, 0, numVertices);
        DbgPrint("dlEndMain: DrawPrimitive hr=0x%08X", hr);
        s_dl.vtxptr += numVertices;
    }
    else
    {
        DbgPrint("dlEndMain: FAIL shader check failed");
    }

    dlData->numvertexes = 0;
    s_dl.constusage = 0;
}

void dlFillRectangleZ(float x1, float y1, float x2, float y2, float z,
    float cr, float cg, float cb, float ca)
{
    // Convert color to packed format
    s_dl.constvtx.col = platformColour(cr, cg, cb, ca);
    s_dl.constusage |= 1;

    // Begin drawing a triangle strip (4 = D3DPT_TRIANGLESTRIP, 4 vertices, 1 = use const color)
    idlContextStruct.ptr = nullptr;
    idlContextStruct.data = &idlContextData;

    float* v = dlBeginMain(&idlContextStruct, 4, 4, 1);
    idlContextStruct.ptr = v;

    if (!v)
        return;

    // Vertex layout: x, y, z (position only - color comes from constvtx)
    // Triangle strip for a quad:
    // v0(x1,y1) --- v1(x2,y1)
    //    |        /     |
    // v2(x1,y2) --- v3(x2,y2)

    *idlContextStruct.ptr++ = x1;
    *idlContextStruct.ptr++ = y1;
    *idlContextStruct.ptr++ = z;

    *idlContextStruct.ptr++ = x2;
    *idlContextStruct.ptr++ = y1;
    *idlContextStruct.ptr++ = z;

    *idlContextStruct.ptr++ = x1;
    *idlContextStruct.ptr++ = y2;
    *idlContextStruct.ptr++ = z;

    *idlContextStruct.ptr++ = x2;
    *idlContextStruct.ptr++ = y2;
    *idlContextStruct.ptr++ = z;

    dlEndMain(idlContextStruct.data, idlContextStruct.ptr);
}

int dword_82C20E48 = 0;
int dword_82C20E4C = 0;

void platformPopMatricesHW()
{
    if (mviewmtxstack.data && mviewmtxstack.cur > 0)
    {
        --mviewmtxstack.cur;
        memcpy(&mviewmtxcur,
            (char*)mviewmtxstack.data + mviewmtxstack.datasize * mviewmtxstack.cur,
            mviewmtxstack.datasize);
    }

    // Pop perspmtxcur from perspmtxstack
    if (perspmtxstack.data && perspmtxstack.cur)
    {
        --perspmtxstack.cur;
        memcpy(&perspmtxcur,
            (char*)perspmtxstack.data + perspmtxstack.datasize * perspmtxstack.cur,
            perspmtxstack.datasize);
    }

    UpdateMVP();
}

int s_minIndex = 0;

void rStatePop()
{
    if (s_topIndex < s_minIndex)
        return;

    rStateBlock* v1 = s_top;
    int v0 = s_topIndex - 1;
    --s_topIndex;
    --s_top;
    g_rState = &s_top->values;

    slinkdef_s* head = v1->changed.head;

    if (v1 != (rStateBlock*)head)
    {
        do
        {
            // Calculate offset of this state slot within the block
            int offset = (int)((char*)head + v1->changed.offset - (char*)v1) - 8;

            if (v0 >= 0)
            {
                // Find the most recently set value for this state in the stack
                int v3 = v0;
                rStateBase* v5 = (rStateBase*)((char*)&s_stack[v0].values + offset);

                while (v5->link.next == (slinkdef_s*)v5)
                {
                    --v3;
                    v5 = (rStateBase*)((char*)v5 - sizeof(rStateBlock));
                    if (v3 < 0)
                        goto next_node;
                }

                setState(v5, offset, v5->numValues);
            }

        next_node:
            slinkdef_s* next = head->next;
            head->next = head; // self-link (unlink from list)
            head = next;

        } while (v1 != (rStateBlock*)head);
    }

    v1->changed.head = (slinkdef_s*)v1;
}

void platformClearBufferHW(int buffermask)
{
    rStateWrite();

    // Check if color buffer clear is requested (bit 0)
    int clearColor = (buffermask & 1) ? 0xF : 0;

    if (clearColor)
    {
        // Save current color mask state
        int savedMask[4];
        savedMask[0] = s_toSend.values.colourmask.i[0];
        savedMask[1] = s_toSend.values.colourmask.i[1];
        savedMask[2] = s_toSend.values.colourmask.i[2];
        savedMask[3] = s_toSend.values.colourmask.i[3];

        // Check if any channel is masked
        bool anyMasked = false;
        for (int i = 0; i < 4; i++)
            if (savedMask[i]) anyMasked = true;
        bool alphaMasked = (savedMask[3] != 0);

        // If all channels unmasked and alpha unmasked, use D3D clear directly
        if (!anyMasked && !alphaMasked)
        {
            clearColor = 0;
        }
        else if (anyMasked || alphaMasked)
        {
            // Need to draw a fullscreen rect to clear with masking
            dlPush2D();

            // Set face cull to no cull (clear lowest bit)
            rStateInt1* p_facecull = (rStateInt1*)g_rState;
            p_facecull->i[0] = s_toSend.values.facecull.i[0] & ~1;
            p_facecull->base.writeCB.writeInt1 = writeFaceCull;
            if (p_facecull->base.link.next == (slinkdef_s*)p_facecull)
            {
                p_facecull->base.link.next = s_top->changed.head;
                s_top->changed.head = &p_facecull->base.link;
            }
            p_facecull->base.vtype = 1;
            p_facecull->base.numValues = 1;
            setState(&p_facecull->base, 0, 1);

            platformDepthTestHW(0);
            platformDepthWriteHW(0);
            platformBlendHW(0);

            // Extract RGBA from clearcolour and convert to float
            uint8_t ca = clearcolour.__s0.a;
            uint8_t cr = clearcolour.__s0.r;
            uint8_t cg = clearcolour.__s0.g;
            uint8_t cb = clearcolour.__s0.b;

            const float inv255 = 1.0f / 255.0f;
            float fa = ca * inv255;
            float fr = cr * inv255;
            float fg = cg * inv255;
            float fb = cb * inv255;

            // Store and disable shaders
            //g_storedProgVP = lastprog_v;
            //g_storedProgFP = lastprog_p;
            //g_shaderEnabledVP = 0;
            //g_shaderEnabledFP = 0;

            // Draw fullscreen colored rectangle
            dlFillRectangleZ(0.0f, 0.0f, 1.0f, 1.0f, 0.0f, fr, fg, fb, fa);

            // Restore shaders
            //shaderSetVS(g_storedProgVP);
            //shaderSetPS(g_storedProgFP);

            platformPopMatricesHW();
            rStatePop();

            clearColor = 0;
        }
    }

    // Build D3D clear flags
    DWORD d3dFlags = 0;
    if (buffermask & 1) d3dFlags |= D3DCLEAR_TARGET;
    if (buffermask & 2) d3dFlags |= D3DCLEAR_ZBUFFER;
    if (clearColor)     d3dFlags |= D3DCLEAR_TARGET;

    if (d3dFlags)
    {
        //pbufferPreRenderNotify(
        //    (buffermask & 2) ? 0 : 1,  // modifyDepth
        //    (buffermask & 1) ? 0 : 1); // modifyColor

        g_pd3dDevice->Clear(
            0, nullptr,
            d3dFlags,
            clearcolour.col,
            cleardepth,
            0);
    }
}

void stackPush(stackheader_s* s, char* entry)
{
    if (!s)
        return;
    if (!s->data)
        return;
    if (s->cur >= s->size)
        return;

    memcpy((char*)s->data + s->datasize * s->cur, entry, s->datasize);
    ++s->cur;
}

void platformSetMatrices2DHW()
{
    // Set mviewmtxcur to identity
    memcpy(&mviewmtxcur, &g_identMtx, sizeof(mtx_u));

    // Set perspmtxcur to 2D orthographic projection matrix
    // Maps [0,1] x [0,1] to clip space
    perspmtxcur.f[0][0] = 2.0f;
    perspmtxcur.f[0][1] = 0.0f;
    perspmtxcur.f[0][2] = 0.0f;
    perspmtxcur.f[0][3] = 0.0f;

    perspmtxcur.f[1][0] = 0.0f;
    perspmtxcur.f[1][1] = -2.0f;
    perspmtxcur.f[1][2] = 0.0f;
    perspmtxcur.f[1][3] = 0.0f;

    perspmtxcur.f[2][0] = 0.0f;
    perspmtxcur.f[2][1] = 0.0f;
    perspmtxcur.f[2][2] = 1.0f;
    perspmtxcur.f[2][3] = 0.0f;

    perspmtxcur.f[3][0] = -1.0f;
    perspmtxcur.f[3][1] = 1.0f;
    perspmtxcur.f[3][2] = 0.0f;
    perspmtxcur.f[3][3] = 1.0f;

    UpdateMVP();
}

void dlPush2D()
{
    // Push render state layer if stack not full
    if (s_topIndex + 1 < 16)
    {
        ++s_topIndex;
        ++s_top;
        g_rState = &s_top->values;
    }

    // Save current matrices
    stackPush(&mviewmtxstack, (char*)&mviewmtxcur);
    stackPush(&perspmtxstack, (char*)&perspmtxcur);

    // Set up 2D orthographic matrices
    platformSetMatrices2DHW();

    // Disable depth test and write for 2D
    platformDepthTestHW(0);
    platformDepthWriteHW(0);

    // Set face cull with lowest bit cleared (no cull)
    rStateValueStore* v0 = g_rState;
    v0->facecull.i[0] = s_toSend.values.facecull.i[0] & ~1;
    v0->facecull.base.writeCB.writeInt1 = writeFaceCull;

    if (v0->facecull.base.link.next == (slinkdef_s*)v0)
    {
        v0->facecull.base.link.next = s_top->changed.head;
        s_top->changed.head = &v0->facecull.base.link;
    }

    v0->facecull.base.vtype = 1;
    v0->facecull.base.numValues = 1;
    setState(&v0->facecull.base, 0, 1);
}

void platformSwapBuffersHW()
{
    // Xbox 360: D3DDevice_SynchronizeToPresentationInterval -- waits for vsync
    // Xbox 360: D3DDevice_Resolve -- resolves MSAA backbuffer to front buffer texture
    // Xbox 360: D3DDevice_Swap -- presents the front buffer
    // All Xbox 360 specific -- on PC just use Present()

    g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);

    // Restore render target and depth stencil
    // On PC these are managed by D3D9 automatically after Present()
    // but set them explicitly to match original behavior
    g_pd3dDevice->SetRenderTarget(0, g_pD3dBackBufferTarget);
    g_pd3dDevice->SetDepthStencilSurface(g_pD3dDepthStencilTarget);
}

//Original
// 
//void dlInitialiseHW()
//{
//    // Skip shader compilation for now
//#if 0
//    // ... shader loop ...
//#endif
//
//    // These are needed for any rendering to work
//    g_pd3dDevice->CreateVertexBuffer(
//        0x2000,
//        D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
//        0,
//        D3DPOOL_DEFAULT,
//        &s_dl.vtxbuffer,
//        nullptr);
//
//    g_pd3dDevice->CreateIndexBuffer(
//        0x2000,
//        D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
//        D3DFMT_INDEX16,
//        D3DPOOL_DEFAULT,
//        &s_dl.indexbuffer,
//        nullptr);
//
//    s_dl.vtxptr = 0;
//    s_dl.indexptr = 0;
//    s_dl.clip_left = 0;
//    s_dl.clip_right = s_initScreenWidth;
//    s_dl.clip_top = 0;
//    s_dl.clip_bottom = s_initScreenHeight;
//    s_dl.boundib = 0;
//    s_dl.initalised = 1;
//    s_dl.bufferptr = 0;
//    s_dl.indexStartFrame = 0;
//    s_dl.constusage = 0;
//
//    void* indexData = malloc(0x200000);
//    s_dl.indexDatabuffer = (unsigned __int8*)indexData;
//    s_dl.indexDataptr = (unsigned __int8*)indexData;
//}

void dlInitialiseHW()
{
    const int numShaderTypes = 18447;
    const size_t shaderTableSize = numShaderTypes * sizeof(void*);

    // Allocate shader/decl tables
    s_pCustomVertexDecls = (IDirect3DVertexDeclaration9**)memAllocAlignCore(shaderTableSize, s_dlHwMemGroup, 0, __FILE__, __LINE__, nullptr, 1);
    s_pCustomVertexShaders = (IDirect3DVertexShader9**)memAllocAlignCore(shaderTableSize, s_dlHwMemGroup, 0, __FILE__, __LINE__, nullptr, 1);
    s_pCustomPixelShaders = (IDirect3DPixelShader9**)memAllocAlignCore(shaderTableSize, s_dlHwMemGroup, 0, __FILE__, __LINE__, nullptr, 1);
    memset(s_pCustomVertexDecls, 0, shaderTableSize);
    memset(s_pCustomVertexShaders, 0, shaderTableSize);
    memset(s_pCustomPixelShaders, 0, shaderTableSize);

    // Pre-compiled vs_3_0 bytecode for:
    // VS_OUT vsmain(VS_IN IN) { OUT.POS = mul(SModelViewProj, IN.POS); OUT.COL = ConstCol; }
    static const DWORD vsCode[] =
    {
        0xFFFE0300,                     // vs_3_0
        0x0200001F, 0x80000000, 0x900F0000, // dcl_position v0
        0x0200001F, 0x80000005, 0xE00F0000, // dcl_position o0
        0x0200001F, 0x8001000A, 0xE00F0001, // dcl_color o1
        0x04000004, 0xC00F0000, 0x90E40000, 0xA0E40000, 0xA0E40001, // m4x4 oPos, v0, c0
        0x02000001, 0xE00F0001, 0xA0E40004, // mov o1, c4
        0x0000FFFF
    };

    // Pre-compiled ps_3_0 bytecode for:
    // float4 psmain(VS_OUT IN) : COLOR { return IN.COL; }
    static const DWORD psCode[] =
    {
        0xFFFF0300,                     // ps_3_0
        0x0200001F, 0x8001000A, 0x900F0000, // dcl_color v0
        0x02000001, 0x800F0800, 0x90E40000, // mov oC0, v0
        0x0000FFFF
    };

    IDirect3DVertexShader9* vs = nullptr;
    IDirect3DPixelShader9* ps = nullptr;
    g_pd3dDevice->CreateVertexShader(vsCode, &vs);
    g_pd3dDevice->CreatePixelShader(psCode, &ps);

    //DbgPrint("CreateVertexShader hr=0x%08X vs=%p", hrVS, vs);
    //DbgPrint("CreatePixelShader  hr=0x%08X ps=%p", hrPS, ps);

    // Store in all slots
    for (int i = 0; i < numShaderTypes; i++)
    {
        s_pCustomVertexShaders[i] = vs;
        s_pCustomPixelShaders[i] = ps;

        D3DVERTEXELEMENT9 decl[17];
        generatedecl(decl, i);
        g_pd3dDevice->CreateVertexDeclaration(decl, &s_pCustomVertexDecls[i]);
    }

    // Allocate vertex and index buffers
    g_pd3dDevice->CreateVertexBuffer(
        0x2000,
        D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
        0,
        D3DPOOL_DEFAULT,
        &s_dl.vtxbuffer,
        nullptr);

    g_pd3dDevice->CreateIndexBuffer(
        0x2000,
        D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
        D3DFMT_INDEX16,
        D3DPOOL_DEFAULT,
        &s_dl.indexbuffer,
        nullptr);

    void* dlBase = memAllocAlignCore(0x600000, s_dlHwMemGroup, 4096, __FILE__, __LINE__, nullptr, 1);
    s_dlBuffer.currentPos = 0;
    s_dlBuffer.size = 0x600000;
    s_dlBuffer.base = (unsigned __int8*)dlBase;

    void* indexData = malloc(0x200000);
    s_dl.indexDatabuffer = (unsigned __int8*)indexData;
    s_dl.indexDataptr = (unsigned __int8*)indexData;

    s_dl.vtxptr = 0;
    s_dl.indexptr = 0;
    s_dl.clip_left = 0;
    s_dl.clip_right = s_initScreenWidth;
    s_dl.clip_top = 0;
    s_dl.clip_bottom = s_initScreenHeight;
    s_dl.boundib = 0;
    s_dl.initalised = 1;
    s_dl.bufferptr = 0;
    s_dl.indexStartFrame = 0;
    s_dl.constusage = 0;
}

void hwInitialise()
{
    texHwInit();

    // Xbox 360: allocates 240MB of physical video memory
    // On PC, D3D manages its own video memory — skip XPhysicalAlloc/hmemInit
    
    //s_baseAddr = nullptr;
    //s_pVidMemContext = nullptr;
    //s_vidMemFreeSpace = 0xF000000;

    InitD3D();

    // Initialize UV test info linked list
    for (int i = 0; i < 17; i++)
        uvtestinfo_static[i].next = &uvtestinfo_static[i + 1];
    uvtestinfo_static[17].next = nullptr;

    //shaderInitCfg4();
    platformStateInitialiseHW();
    
    //SHADERS TODO
    dlInitialiseHW();
    
    occQueryInitialise();

    stackInit(&mviewmtxstack, 32, 64);
    stackInit(&perspmtxstack, 32, 64);

    // Copy identity matrix into mviewmtxcur and perspmtxcur
    memcpy(&mviewmtxcur, &g_identMtx, sizeof(mtx_u));
    memcpy(&perspmtxcur, &g_identMtx, sizeof(mtx_u));

    UpdateMVP();

    // Xbox 360: XNotifyCreateListener is a Live/system notification API — skip on PC
    //s_notificationHandler.m_listener = 0;
    //s_notificationHandler.m_bSigninChanged = 0;
    //s_notificationHandler.m_bStorageDevicesChanged = 0;
    //s_notificationHandler.m_bMuteListChanged = 0;
    //s_notificationHandler.m_bLiveConnectionChanged = 0;
    //s_notificationHandler.m_LiveInviteAcceptedPlayerIndex = 0xFF;
    //s_notificationHandler.m_bSigninChangedReceived = 0;
    //s_notificationHandler.m_bStorageDevicesChangedReceived = 0;
    //s_notificationHandler.m_bShowingSystemUI = 0xFF;

    texHwMemCopyOnGpuInit();

    // Clear to purple (r=0.3, g=0.1, b=0.4, a=0.0) and swap twice
    platformClearColourHW(0.3f, 0.1f, 0.4f, 0.0f);
    for (int i = 0; i < 2; i++)
    {
        platformClearBufferHW(1);
        platformSwapBuffersHW();
    }

    // Set up clear colour render state
    rStateFloat4* p_clearcolour = &g_rState->clearcolour;
    p_clearcolour->f[0] = 0.0f;
    p_clearcolour->f[1] = 0.0f;
    p_clearcolour->f[2] = 0.0f;
    p_clearcolour->f[3] = 0.0f;
    p_clearcolour->base.writeCB.writeFloat4 = writeClearColour;
    p_clearcolour->base.vtype = 0x400;
    p_clearcolour->base.numValues = 4;

    // Link into render state changed list if not already linked
    if (p_clearcolour->base.link.next == (slinkdef_s*)p_clearcolour)
    {
        p_clearcolour->base.link.next = s_top->changed.head;
        s_top->changed.head = &p_clearcolour->base.link;
    }

    setState(&p_clearcolour->base,
        (char*)p_clearcolour - (char*)g_rState,
        4);
}






dlVertexDataInfo dlVtxDataInfo[30] =
{
  { 2u, 2u, 0u, 0u, '\0' },
  { 3u, 3u, 0u, 0u, '\0' },
  { 4u, 4u, 0u, 0u, '\0' },
  { 2u, 4u, 2u, 0u, '\0' },
  { 2u, 4u, 1u, 0u, '\0' },
  { 4u, 4u, 0u, 1u, '\0' },
  { 2u, 4u, 1u, 1u, '\0' },
  { 1u, 4u, 3u, 1u, '\0' },
  { 3u, 3u, 0u, 2u, '\0' },
  { 2u, 2u, 0u, 3u, '\0' },
  { 1u, 2u, 2u, 3u, '\0' },
  { 3u, 3u, 0u, 3u, '\0' },
  { 4u, 4u, 0u, 3u, '\0' },
  { 2u, 4u, 1u, 3u, '\0' },
  { 2u, 2u, 0u, 3u, '\x01' },
  { 3u, 3u, 0u, 3u, '\x01' },
  { 4u, 4u, 0u, 3u, '\x01' },
  { 2u, 4u, 1u, 3u, '\x01' },
  { 2u, 2u, 0u, 3u, '\x02' },
  { 3u, 3u, 0u, 3u, '\x02' },
  { 4u, 4u, 0u, 3u, '\x02' },
  { 2u, 4u, 1u, 3u, '\x02' },
  { 2u, 2u, 0u, 3u, '\x03' },
  { 3u, 3u, 0u, 3u, '\x03' },
  { 4u, 4u, 0u, 3u, '\x03' },
  { 2u, 4u, 1u, 3u, '\x03' },
  { 2u, 2u, 0u, 3u, '\x04' },
  { 3u, 3u, 0u, 3u, '\x04' },
  { 4u, 4u, 0u, 3u, '\x04' },
  { 2u, 4u, 1u, 3u, '\x04' }
};

int generatedecl(D3DVERTEXELEMENT9* decl, int type)
{
    // D3D type lookup tables
    static const DWORD typeTableFloat[] = { 0x2C83A4, 0x2C23A5, 0x2A23B9, 0x1A23A6 }; // float1-4
    static const DWORD typeTableColor[] = { 0x2C2359, 0x2C2359, 0x2C2359, 0x1A235A }; // color types

    int    result = 0;
    int    byteOffset = 0;
    int    v6 = 29;
    unsigned __int8* p = &dlVtxDataInfo[29].numElems;
    D3DVERTEXELEMENT9* v8 = decl;

    do
    {
        if ((1 << v6) & dlVtxInfo[type].requiredFlags)
        {
            v8->Stream = 0;
            v8->Method = 0;
            v8->Offset = (WORD)(byteOffset * 4);

            switch (p[2])
            {
            case 0: // POSITION
            {
                DWORD d3dType = (p[1] == 2)
                    ? typeTableFloat[(*p * 4) >> 2]
                    : typeTableColor[(*p * 4) >> 2];
                v8->Usage = 0;
                v8->UsageIndex = 0;
                v8->Type = (BYTE)d3dType;
                ++v8; ++result;
                break;
            }
            case 1: // COLOR
            {
                if (p[1])
                {
                    v8->Usage = 10;
                    v8->UsageIndex = 0;
                    v8->Type = 0x182886;
                }
                else
                {
                    DWORD d3dType = typeTableColor[(*p * 4) >> 2];
                    v8->Usage = 10;
                    v8->UsageIndex = 0;
                    v8->Type = (BYTE)d3dType;
                }
                ++v8; ++result;
                break;
            }
            case 2: // NORMAL
            {
                DWORD d3dType = typeTableColor[(*p * 4) >> 2];
                v8->Usage = 3;
                v8->UsageIndex = 0;
                v8->Type = (BYTE)d3dType;
                ++v8; ++result;
                break;
            }
            case 3: // TEXCOORD
            {
                DWORD d3dType = (p[1] == 2)
                    ? typeTableFloat[(*p * 4) >> 2]
                    : typeTableColor[(*p * 4) >> 2];
                v8->Usage = 5;
                v8->Type = (BYTE)d3dType;
                v8->UsageIndex = p[3];
                ++v8; ++result;
                break;
            }
            default:
                break;
            }

            byteOffset += *((DWORD*)p - 1);
        }

        p -= 8;
        --v6;
    } while ((int)p >= (int)&dlVtxDataInfo[0].numElems);

    // D3DDECL_END()
    v8->Stream = 0xFF;
    v8->Offset = 0;
    v8->Type = (BYTE)-1;
    v8->Method = 0;
    v8->Usage = 0;
    v8->UsageIndex = 0;

    return result;
}

int dlVBCreate(int num, int vt)
{
    // Initialize VB table if needed
    if (!s_bDlVBsInitialised)
    {
        s_pDlVertexBuffers = (vbinfo*)memAllocAlignCore(
            0x4B0, s_dlHwMemGroup, 0, __FILE__, 1278, nullptr, 1);

        vbinfo* v5 = s_pDlVertexBuffers;
        for (int i = 0; i < 150; i++)
        {
            v5->m_vb = nullptr;
            ++v5;
        }
        s_bDlVBsInitialised = 1;
    }

    // Find a free slot (skip slot 0, start at 1)
    int v7 = 1;
    vbinfo* slot = s_pDlVertexBuffers + 1;
    while (slot->m_vb)
    {
        ++v7;
        ++slot;
        if (v7 >= 150)
            return 0;
    }

    if (v7 <= 0)
        return 0;

    // Create vertex buffer
    UINT size = dlVtxInfo[vt].sizeInFloats * num * sizeof(float);
    IDirect3DVertexBuffer9* vb = nullptr;
    g_pd3dDevice->CreateVertexBuffer(
        size,
        D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
        0,
        D3DPOOL_DEFAULT,
        &vb,
        nullptr);

    if (!vb)
        return 0;

    // Store in slot
    s_pDlVertexBuffers[v7].m_vb = vb;
    s_pDlVertexBuffers[v7].m_vt = vt;

    // Create vertex declaration if not already created
    if (!s_pCustomVertexDecls[vt])
    {
        D3DVERTEXELEMENT9 decl[21];
        generatedecl(decl, vt);
        g_pd3dDevice->CreateVertexDeclaration(decl, &s_pCustomVertexDecls[vt]);
    }

    return v7;
}


unsigned __int8 s_bDlIBsInitialised = 0u;

int dlIBCreate(unsigned int sizeinbytes)
{
    // Initialize IB table if needed
    if (!s_bDlIBsInitialised)
    {
        s_pDlIndexBuffers = (ibinfo*)memAllocAlignCore(
            0x3E80, s_dlHwMemGroup, 0, __FILE__, 1412, nullptr, 1);

        for (int i = 0; i < 4000; i++)
            s_pDlIndexBuffers[i].m_ib = nullptr;

        s_bDlIBsInitialised = 1;
    }

    // Find a free slot (skip slot 0, start at 1)
    int v5 = 1;
    ibinfo* slot = s_pDlIndexBuffers + 1;
    while (slot->m_ib)
    {
        ++v5;
        ++slot;
        if (v5 >= 4000)
            return 0;
    }

    if (v5 <= 0)
        return 0;

    // Create index buffer
    IDirect3DIndexBuffer9* ib = nullptr;
    g_pd3dDevice->CreateIndexBuffer(
        sizeinbytes,
        D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
        D3DFMT_INDEX16,
        D3DPOOL_DEFAULT,
        &ib,
        nullptr);

    if (!ib)
        return 0;

    s_pDlIndexBuffers[v5].m_ib = ib;
    return v5;
}

PBuffer_s pbuffers[64];
unsigned __int8* s_wodge = NULL;
PBufferHW_s* s_pPBuffersHW = NULL;
int s_wodgeMax = 0;

void pbufferReleaseHW(unsigned int handle)
{
    if (handle == (unsigned int)-1 || handle >= 0x40)
        return;

    PBufferHW_s* v5 = &s_pPBuffersHW[handle];
    PBuffer_s* v6 = &pbuffers[handle];

    // Release D3D surfaces
    if (v5->rendertarget)
        v5->rendertarget->Release();
    if (v5->renderdepthstencil)
        v5->renderdepthstencil->Release();

    v5->rendertarget = nullptr;
    v5->renderdepthstencil = nullptr;

    // Free textures
    if (v6->colorTexture != (unsigned int)-1)
        texFreeTexture(v6->colorTexture);
    if (v6->depthTexture != (unsigned int)-1)
        texFreeTexture(v6->depthTexture);

    // Reset pbuffer slot
    v6->width = 0;
    v6->height = 0;
    v6->valid = 0;
    v6->colorTexture = -1;
    v6->depthTexture = -1;
}

unsigned int pbufferCreate(int w, int h, int bpc, const PBufferOption* options)
{
    int  v4 = h;
    int  v6 = 0;
    unsigned int v7 = 0;
    unsigned int v8 = 24;
    bool v9 = false;
    bool v10 = false;
    bool v11 = false;
    bool v45 = false;

    // Find free pbuffer slot
    unsigned int v12 = 0;
    PBuffer_s* v13 = pbuffers;
    while (v13->valid)
    {
        ++v13;
        ++v12;
        if ((int)v13 >= (int)&s_wodge)
            return -1;
    }

    unsigned int   v15 = v12;
    PBuffer_s* v16 = &pbuffers[v12];
    PBufferHW_s* v17 = s_pPBuffersHW;
    PBufferHW_s* v18 = &s_pPBuffersHW[v12];

    if (!v16)
        return v15;

    v18->rendertarget = nullptr;
    v18->renderdepthstencil = nullptr;

    // Parse options
    if (options && options->type)
    {
        const PBufferOption* opt = options;
        PBufferOptionType type = opt->type;
        const unsigned int* p_value = (const unsigned int*)& opt->value;

        do
        {
            switch (type)
            {
            case PBUFFER_FLOAT_BUFFER:                        if (*p_value) v6 |= 0x80;   break;
            case PBUFFER_SHARE_BUFFER:                        if (*p_value) v6 |= 0x20;   break;
            case PBUFFER_CREATE_COLOR_TEXTURE:                v10 = (*p_value != 0);       break;
            case PBUFFER_CREATE_DEPTH_TEXTURE:                v45 = (*p_value != 0);       break;
            case PBUFFER_DEPTH_ONLY:                          v9 = (*p_value != 0);       break;
            case PBUFFER_DEPTH_COMPARE:                       if (*p_value) v6 |= 0x8;    break;
            case PBUFFER_STENCIL_BITS:                        v7 = *p_value;              break;
            case PBUFFER_DEPTH_BITS:                          v8 = *p_value;              break;
            case PBUFFER_USE_TEX2D:                           if (*p_value) v6 |= 0x1;    break;
            case PBUFFER_DISABLE_FILTERING:                   if (*p_value) v6 |= 0x2;    break;
            case PBUFFER_COLOR_TEXTURE_DISABLE_FILTERING:     if (*p_value) v6 |= 0x200;  break;
            case PBUFFER_DEPTH_TEXTURE_DISABLE_FILTERING:     if (*p_value) v6 |= 0x400;  break;
            case PBUFFER_TEXTURE_CLAMP:                       if (*p_value) v6 |= 0x40;   break;
            case PBUFFER_LUM_BUFFER:                          if (*p_value) v6 |= 0x100;  break;
            case PBUFFER_XENON_NOT_OVERLAPING_DL_PBUFFER_FRAME: v11 = (*p_value != 0);    break;
            default: break;
            }
            p_value += 2;
            type = (PBufferOptionType) * (p_value - 1);
        } while (type);
    }

    bool v21 = v10;

    // Align dimensions if creating textures
    if (v10 || v45)
    {
        w = (w + 7) & ~7;
        v4 = (v4 + 7) & ~7;
    }

    v16->width = w;
    v16->height = v4;
    v16->colorTexture = -1;
    v16->depthTexture = -1;

    v18->bColourResolvedOnLastDeactivate = 1;
    v18->bDepthResolvedOnLastDeactivate = 1;
    v18->bColorRenderedAfterLastLastActivate = 1;
    v18->bDepthRenderedAfterLastLastActivate = 1;

    // Select color format
    D3DFORMAT v22;
    if (v6 & 0x80)
        v22 = (v6 & 0x100) ? D3DFMT_R32F : D3DFMT_A2R10G10B10;
    else if (bpc < 16)
        v22 = D3DFMT_A8R8G8B8;
    else
        v22 = D3DFMT_A16B16G16R16F;

    // Select depth format
    D3DFORMAT v23;
    if (v7)
        v23 = D3DFMT_D24S8;
    else if (v8 > 0x18)
        v23 = D3DFMT_D32;
    else if (v8 > 0x10)
        v23 = D3DFMT_D24X8;
    else
        v23 = D3DFMT_D16;

    // Try to share an existing pbuffer
    bool sharedFound = false;
    if (v6 & 0x20)
    {
        int v24 = 0;
        for (int i = 0; i < (int)((int)&s_wodgeMax - (int)pbuffers) / sizeof(PBuffer_s); i++)
        {
            if (pbuffers[i].valid &&
                pbuffers[i].width >= w &&
                pbuffers[i].height >= v4 &&
                pbuffers[i].bpc >= bpc &&
                v17[i].renderformat == v22 &&
                v17[i].depthformat == v23)
            {
                v24 = i;
                sharedFound = true;
                break;
            }
        }

        if (sharedFound && v24 != -1)
        {
            PBufferHW_s* v36 = &v17[v24];
            v18->renderformat = v36->renderformat;
            v18->rendertarget = v36->rendertarget;
            v18->renderdepthstencil = v36->renderdepthstencil;
            g_pd3dDevice->AddRef(); // D3DResource_AddRef equivalents
            g_pd3dDevice->AddRef();
        }
    }

    if (!sharedFound)
    {
        // Create render target surface
        if (!v9)
        {
            IDirect3DSurface9* surface = nullptr;
            g_pd3dDevice->CreateRenderTarget(w, v4, v22, D3DMULTISAMPLE_NONE, 0, FALSE, &surface, nullptr);
            v18->rendertarget = surface;
            if (!surface)
                goto fail;
        }

        // Create depth stencil surface
        if (v8 || v7)
        {
            IDirect3DSurface9* depth = nullptr;
            g_pd3dDevice->CreateDepthStencilSurface(w, v4, v23, D3DMULTISAMPLE_NONE, 0, FALSE, &depth, nullptr);
            v18->renderdepthstencil = depth;
            if (!depth)
                goto fail;
        }
    }

    // Create color texture
    if (v21)
    {
        TexFlags v41 = TEX_FLAG_PERMANENT;
        if (v6 & 0x40)  v41 = (TexFlags)(v41 | TEX_FLAG_CLAMPUV);
        if ((v6 & 0x200) || (v6 & 0x2)) v41 = (TexFlags)(v41 | 0x10);
        if (v6 & 0x80)
        {
            v41 = (TexFlags)(v41 | 0x800);
            if (v6 & 0x100) v41 = (TexFlags)(v41 | 0x1000);
        }
        unsigned int rt = texCreateRenderTexture(w, v4, v41);
        v16->colorTexture = rt;
        if (rt == (unsigned int)-1)
            goto fail;
    }

    // Create depth texture
    if (v45)
    {
        TexFlags v43 = (TexFlags)(TEX_FLAG_PERMANENT | TEX_FLAG_DEPTH);
        if ((v6 & 0x400) || (v6 & 0x2))
            v43 = (TexFlags)(TEX_FLAG_PERMANENT | TEX_FLAG_NEAREST | TEX_FLAG_DEPTH);
        unsigned int dt = texCreateRenderTexture(w, v4, v43);
        v16->depthTexture = dt;
        if (dt == (unsigned int)-1)
            goto fail;
    }

    v16->valid = 1;
    v16->bFloat = (v6 & 0x80) != 0;
    return v15;

fail:
    pbufferReleaseHW(v15);
    return -1;
}

void frameResetRenderState()
{
    platformClearColourHW(0.0f, 0.0f, 0.0f, 1.0f);
    platformClearDepthHW(1.0f);
    platformDepthTestHW(1);
    platformDepthFuncHW(21);
    platformDepthWriteHW(1);

    // Set face cull to 3 (CCW)
    rStateValueStore* v1 = g_rState;
    v1->facecull.i[0] = 3;
    v1->facecull.base.writeCB.writeInt1 = writeFaceCull;
    if (v1->facecull.base.link.next == (slinkdef_s*)v1)
    {
        v1->facecull.base.link.next = s_top->changed.head;
        s_top->changed.head = &v1->facecull.base.link;
    }
    v1->facecull.base.vtype = 1;
    v1->facecull.base.numValues = 1;
    setState(&v1->facecull.base, 0, 1);

    platformDepthBias(0.0f, 0.0f);
    platformWireframeHW(0);
    platformBlendHW(0);
    dlScissorTest(0);

    int w = (int)windowfullscreen.width;
    int h = (int)windowfullscreen.height;
    platformScissorGL(0, 0, w, h);
    platformViewportGL(0, 0, w, h);

    dlSetPointSize(1.0f);
    dlSetLineWidth(1.0f);
    platformAlphaTestHW(0, 0);
    platformShadeHW(28);
    platformStencilTestHW(0);
    platformStencilFuncHW(29, 0, 0);
    platformStencilOpHW(36, 36, 36);
    platformColourMaskHW(1, 1, 1, 0);
    platformDepthWriteHW(1);
}

void dlSetFrameBuffers()
{
    int w = (int)windowfullscreen.width;
    int h = (int)windowfullscreen.height;

    if (w != sFrameSizeX || h != sFrameSizeY)
    {
        sFrameSizeX = w;
        sFrameSizeY = h;

        // Build option lists for pbufferCreate
        static const PBufferOption frameOptions[] =
        {
            { PBUFFER_USE_TEX2D,                    1 },
            { PBUFFER_CREATE_COLOR_TEXTURE,         1 },
            { PBUFFER_CREATE_DEPTH_TEXTURE,         1 },
            { PBUFFER_DEPTH_BITS,                   16 },
            { PBUFFER_STENCIL_BITS,                 1 },
            { PBUFFER_DEPTH_COMPARE,                1 },
            { PBUFFER_DISABLE_FILTERING,            0 },
            { PBUFFER_DEPTH_ONLY,                   0 },
            { PBUFFER_TEXTURE_CLAMP,                0 },
            { (PBufferOptionType)0,                 0 }
        };

        static const PBufferOption scratchOptions[] =
        {
            { PBUFFER_USE_TEX2D,                    1 },
            { PBUFFER_CREATE_COLOR_TEXTURE,         1 },
            { PBUFFER_DISABLE_FILTERING,            0 },
            { PBUFFER_DEPTH_ONLY,                   0 },
            { PBUFFER_TEXTURE_CLAMP,                0 },
            { (PBufferOptionType)0,                 0 }
        };

        static const PBufferOption quarterOptions[] =
        {
            { PBUFFER_USE_TEX2D,                    1 },
            { PBUFFER_CREATE_COLOR_TEXTURE,         1 },
            { PBUFFER_DEPTH_BITS,                   16 },
            { PBUFFER_STENCIL_BITS,                 1 },
            { PBUFFER_DISABLE_FILTERING,            0 },
            { PBUFFER_DEPTH_ONLY,                   0 },
            { PBUFFER_TEXTURE_CLAMP,                0 },
            { (PBufferOptionType)0,                 0 }
        };

        // Release existing pbuffers if valid and not the frame buffer
        auto tryRelease = [](unsigned int handle)
            {
                if (handle != (unsigned int)-1
                    && handle != (unsigned int)sFrameBuffer
                    && handle < 0x40
                    && pbuffers[handle].valid)
                {
                    pbufferReleaseHW(handle);
                }
            };

        tryRelease(DL_PBUFFER_FRAME);
        tryRelease(DL_PBUFFER_FRAME_SCRATCH);
        tryRelease(DL_PBUFFER_FRAME_HALF);
        tryRelease(DL_PBUFFER_FRAME_QUARTER);

        // Create full resolution buffers
        DL_PBUFFER_FRAME = pbufferCreate(w, h, 8, frameOptions);
        DL_PBUFFER_FRAME_SCRATCH = pbufferCreate(w, h, 8, scratchOptions);

        // Create half resolution buffer
        DL_PBUFFER_FRAME_HALF = pbufferCreate(w / 2, h / 2, 8, frameOptions);

        // Create quarter resolution buffer
        DL_PBUFFER_FRAME_QUARTER = pbufferCreate(w / 4, h / 4, 8, quarterOptions);

        s_multisample = 0;
    }

    // Extract texture handles from pbuffers
    auto getColorTex = [](unsigned int handle) -> unsigned int
        {
            if (handle < 0x40 && pbuffers[handle].valid)
                return pbuffers[handle].colorTexture;
            return (unsigned int)-1;
        };

    auto getDepthTex = [](unsigned int handle) -> unsigned int
        {
            if (handle < 0x40 && pbuffers[handle].valid)
                return pbuffers[handle].depthTexture;
            return (unsigned int)-1;
        };

    DL_TEXTURE_FRAME = getColorTex(DL_PBUFFER_FRAME);
    DL_TEXTURE_DEPTH = getDepthTex(DL_PBUFFER_FRAME);
    DL_TEXTURE_FRAME_SCRATCH = getColorTex(DL_PBUFFER_FRAME_SCRATCH);
    DL_TEXTURE_FRAME_QUARTER = getColorTex(DL_PBUFFER_FRAME_QUARTER);
    DL_TEXTURE_FRAME_HALF = getColorTex(DL_PBUFFER_FRAME_HALF);
}

int vecnormaliseGold(vec3_u* vout, const vec3_u* vin)
{
    float x = vin->v[0];
    float y = vin->v[1];
    float z = vin->v[2];

    float len = sqrtf(x * x + y * y + z * z);

    if (len <= 0.0000001f)
    {
        vout->v[0] = x;
        vout->v[1] = y;
        vout->v[2] = z;
        return 0;
    }

    float inv = 1.0f / len;
    vout->v[0] = x * inv;
    vout->v[1] = y * inv;
    vout->v[2] = z * inv;
    return 1;
}

void camFindUpRightVectors(vec3_u* dir, vec3_u* up, vec3_u* right)
{
    float dx = dir->v[0];
    float dy = dir->v[1];
    float dz = dir->v[2];

    float ux = up->v[0];
    float uy = up->v[1];
    float uz = up->v[2];

    // right = cross(up, dir)
    right->v[0] = uy * dz - dy * uz;
    right->v[1] = dx * uz - ux * dz;
    right->v[2] = dy * ux - uy * dx;

    float rx = right->v[0];
    float ry = right->v[1];
    float rz = right->v[2];

    // up = cross(right, dir) -- reorthogonalize up
    up->v[0] = ry * dz - rz * dy;
    up->v[1] = rz * dx - rx * dz;
    up->v[2] = rx * dy - ry * dx;

    // Normalize all three
    vecnormaliseGold(dir, dir);
    vecnormaliseGold(right, right);
    vecnormaliseGold(up, up);
}

void matrixPerspective(mtx_u* m, float fovy, float aspect, float zn, float zf)
{
    float halfFov = fovy * 0.5f;
    float cosHalf = cosf(halfFov);
    float sinHalf = sinf(halfFov);
    float cot = cosHalf / sinHalf; // cotangent = cos/sin
    float q = zf / (zf - zn);

    // Clear matrix
    m->f[0][1] = 0.0f; m->f[0][2] = 0.0f; m->f[0][3] = 0.0f;
    m->f[1][0] = 0.0f; m->f[1][2] = 0.0f; m->f[1][3] = 0.0f;
    m->f[2][0] = 0.0f; m->f[2][1] = 0.0f;
    m->f[3][0] = 0.0f; m->f[3][1] = 0.0f; m->f[3][3] = 0.0f;

    // Set projection values
    m->f[0][0] = cot / aspect;  // x scale
    m->f[1][1] = cot;           // y scale
    m->f[2][2] = -q;            // z scale
    m->f[2][3] = -1.0f;         // w = -z (perspective divide)
    m->f[3][2] = -(q * zn);     // z translation
}

void matrixOrtho(mtx_u* mat, float left, float right, float bottom, float top, float zNear, float zFar)
{
    float rl = right - left;
    float tb = top - bottom;
    float fn = zFar - zNear;

    // Clear off-diagonal elements
    mat->f[0][1] = 0.0f; mat->f[0][2] = 0.0f; mat->f[0][3] = 0.0f;
    mat->f[1][0] = 0.0f; mat->f[1][2] = 0.0f; mat->f[1][3] = 0.0f;
    mat->f[2][0] = 0.0f; mat->f[2][1] = 0.0f; mat->f[2][3] = 0.0f;
    mat->f[3][3] = 1.0f;

    // Scale
    mat->f[0][0] = 2.0f / rl;
    mat->f[1][1] = 2.0f / tb;
    mat->f[2][2] = -1.0f / fn;

    // Translation
    mat->f[3][0] = -(left + right) / rl;
    mat->f[3][1] = -(bottom + top) / tb;
    mat->f[3][2] = -(zNear) / fn;
}

void matrixLook(mtx_u* m, const vec3_u* pos, const vec3_u* dir, const vec3_u* up, const vec3_u* right)
{
    // Rotation part -- note negated dir and right columns (right-handed to left-handed)
    m->f[0][0] = -right->v[0];  m->f[0][1] = up->v[0];  m->f[0][2] = -dir->v[0];  m->f[0][3] = 0.0f;
    m->f[1][0] = -right->v[1];  m->f[1][1] = up->v[1];  m->f[1][2] = -dir->v[1];  m->f[1][3] = 0.0f;
    m->f[2][0] = -right->v[2];  m->f[2][1] = up->v[2];  m->f[2][2] = -dir->v[2];  m->f[2][3] = 0.0f;

    // Translation part -- dot(pos, axis) for each axis
    m->f[3][0] = (pos->v[0] * right->v[0] + pos->v[1] * right->v[1] + pos->v[2] * right->v[2]);
    m->f[3][1] = -(pos->v[0] * up->v[0] + pos->v[1] * up->v[1] + pos->v[2] * up->v[2]);
    m->f[3][2] = (pos->v[0] * dir->v[0] + pos->v[1] * dir->v[1] + pos->v[2] * dir->v[2]);
    m->f[3][3] = 1.0f;
}

void matrixInvert(mtx_u* m, const mtx_u* m1)
{
    // Use temp buffer if m == m1 (in-place invert)
    mtx_u tmp;
    mtx_u* out = (m != m1) ? m : &tmp;

    float a00 = m1->f[0][0], a01 = m1->f[0][1], a02 = m1->f[0][2];
    float a10 = m1->f[1][0], a11 = m1->f[1][1], a12 = m1->f[1][2];
    float a20 = m1->f[2][0], a21 = m1->f[2][1], a22 = m1->f[2][2];
    float a30 = m1->f[3][0], a31 = m1->f[3][1], a32 = m1->f[3][2];

    // Compute determinant of upper-left 3x3
    float det = a00 * (a11 * a22 - a12 * a21)
        - a01 * (a10 * a22 - a12 * a20)
        + a02 * (a10 * a21 - a11 * a20);

    if (det == 0.0f)
        return;

    float inv = 1.0f / det;

    // 3x3 cofactor matrix (transposed = adjugate)
    out->f[0][0] = (a11 * a22 - a12 * a21) * inv;
    out->f[0][1] = (a02 * a21 - a01 * a22) * inv;
    out->f[0][2] = (a01 * a12 - a02 * a11) * inv;
    out->f[0][3] = 0.0f;

    out->f[1][0] = (a12 * a20 - a10 * a22) * inv;
    out->f[1][1] = (a00 * a22 - a02 * a20) * inv;
    out->f[1][2] = (a02 * a10 - a00 * a12) * inv;
    out->f[1][3] = 0.0f;

    out->f[2][0] = (a10 * a21 - a11 * a20) * inv;
    out->f[2][1] = (a01 * a20 - a00 * a21) * inv;
    out->f[2][2] = (a00 * a11 - a01 * a10) * inv;
    out->f[2][3] = 0.0f;

    // Translation row -- multiply original translation by inverted 3x3
    out->f[3][0] = -(a30 * out->f[0][0] + a31 * out->f[1][0] + a32 * out->f[2][0]);
    out->f[3][1] = -(a30 * out->f[0][1] + a31 * out->f[1][1] + a32 * out->f[2][1]);
    out->f[3][2] = -(a30 * out->f[0][2] + a31 * out->f[1][2] + a32 * out->f[2][2]);
    out->f[3][3] = 1.0f;

    // Copy back if in-place
    if (m == m1)
        memcpy(m, &tmp, sizeof(mtx_u));
}

void camCalculateFrustum(frustumdef_s* frustum, const mtx_u* mvp)
{
    // Left plane
    frustum->planeData[0].___u0.plane.v[0] = mvp->f[0][0] + mvp->f[0][3];
    frustum->planeData[0].___u0.plane.v[1] = mvp->f[1][0] + mvp->f[1][3];
    frustum->planeData[0].___u0.plane.v[2] = mvp->f[2][0] + mvp->f[2][3];
    frustum->planeData[0].___u0.plane.v[3] = mvp->f[3][0] + mvp->f[3][3];

    // Right plane
    frustum->planeData[1].___u0.plane.v[0] = mvp->f[0][3] - mvp->f[0][0];
    frustum->planeData[1].___u0.plane.v[1] = mvp->f[1][3] - mvp->f[1][0];
    frustum->planeData[1].___u0.plane.v[2] = mvp->f[2][3] - mvp->f[2][0];
    frustum->planeData[1].___u0.plane.v[3] = mvp->f[3][3] - mvp->f[3][0];

    // Top plane
    frustum->planeData[2].___u0.plane.v[0] = mvp->f[0][3] - mvp->f[0][1];
    frustum->planeData[2].___u0.plane.v[1] = mvp->f[1][3] - mvp->f[1][1];
    frustum->planeData[2].___u0.plane.v[2] = mvp->f[2][3] - mvp->f[2][1];
    frustum->planeData[2].___u0.plane.v[3] = mvp->f[3][3] - mvp->f[3][1];

    // Bottom plane
    frustum->planeData[3].___u0.plane.v[0] = mvp->f[0][3] + mvp->f[0][1];
    frustum->planeData[3].___u0.plane.v[1] = mvp->f[1][3] + mvp->f[1][1];
    frustum->planeData[3].___u0.plane.v[2] = mvp->f[2][1] + mvp->f[2][3];
    frustum->planeData[3].___u0.plane.v[3] = mvp->f[3][1] + mvp->f[3][3];

    // Near plane
    frustum->planeData[4].___u0.plane.v[0] = mvp->f[0][2] + mvp->f[0][3];
    frustum->planeData[4].___u0.plane.v[1] = mvp->f[1][2] + mvp->f[1][3];
    frustum->planeData[4].___u0.plane.v[2] = mvp->f[2][2] + mvp->f[2][3];
    frustum->planeData[4].___u0.plane.v[3] = mvp->f[3][2] + mvp->f[3][3];

    // Far plane
    frustum->planeData[5].___u0.plane.v[0] = mvp->f[0][3] - mvp->f[0][2];
    frustum->planeData[5].___u0.plane.v[1] = mvp->f[1][3] - mvp->f[1][2];
    frustum->planeData[5].___u0.plane.v[2] = mvp->f[2][3] - mvp->f[2][2];
    frustum->planeData[5].___u0.plane.v[3] = mvp->f[3][3] - mvp->f[3][2];

    // Normalize each plane and compute sign bits
    for (int i = 0; i < 6; i++)
    {
        frustumdatadef_s* p = &frustum->planeData[i];

        float nx = p->___u0.plane.v[0];
        float ny = p->___u0.plane.v[1];
        float nz = p->___u0.plane.v[2];
        float d = p->___u0.plane.v[3];

        float lenSq = nx * nx + ny * ny + nz * nz;
        float inv = 0.0f;
        if (lenSq > 0.000001f)
            inv = 1.0f / sqrtf(lenSq);

        p->___u0.plane.v[0] = nx * inv;
        p->___u0.plane.v[1] = ny * inv;
        p->___u0.plane.v[2] = nz * inv;
        p->___u0.plane.v[3] = d * inv;

        p->nx = (p->___u0.plane.v[0] > 0.0f) ? 1 : 0;
        p->ny = (p->___u0.plane.v[1] > 0.0f) ? 1 : 0;
        p->nz = (p->___u0.plane.v[2] > 0.0f) ? 1 : 0;
    }
}

void camCalculateView(viewdef_s* view)
{
    // Calculate pixel dimensions of viewport
    float pixelW = view->window.width * windowfullscreen.width;
    float pixelH = view->window.height * windowfullscreen.height;

    // Calculate aspect ratio
    float aspect = 1.0f;
    if (pixelW > 1.0f && pixelH > 1.0f)
        aspect = pixelW / pixelH;

    // Find camera basis vectors from direction
    camFindUpRightVectors(&view->dir, &view->up, &view->right);

    // Build projection matrix
    switch (view->camprojmode)
    {
    case 1: // perspective
        matrixPerspective(
            &view->perspmtx,
            view->fovRadians,
            aspect * view->window.pixelaspect,
            view->minz,
            view->maxz);
        break;
    case 0: // orthographic
        matrixOrtho(
            &view->perspmtx,
            -view->ortho[0],
            view->ortho[0],
            -view->ortho[1],
            view->ortho[1],
            view->minz,
            view->maxz);
        break;
    default:
        break;
    }

    // Build view matrices
    matrixLook(&view->modelviewmtx, &view->pos, &view->dir, &view->up, &view->right);
    matrixInvert(&view->invmodelviewmtx, &view->modelviewmtx);
    matrixMultiplyAligned(&view->modelviewperspmtx, &view->modelviewmtx, &view->perspmtx);

    // Transpose the invtransmodelviewpersmtx -- VMX transpose replaced with memcpy transpose
    matrixInvert(&view->invtransmodelviewpersmtx, &view->modelviewperspmtx);

    // Transpose invtransmodelviewpersmtx in place (VMX vmrghw/vmrglw = matrix transpose)
    mtx_u* m = &view->invtransmodelviewpersmtx;
    mtx_u tmp;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            tmp.f[i][j] = m->f[j][i];
    memcpy(m, &tmp, sizeof(mtx_u));

    camCalculateFrustum(&view->frustum, &view->invtransmodelviewpersmtx);
}

viewcache dlviewcache;

void matrixToQuaternion(quaternion_u* q, const mtx_u* m)
{
    static const int s_next[3] = { 1, 2, 0 };

    float trace = m->f[0][0] + m->f[1][1] + m->f[2][2];

    if (trace > 0.0f)
    {
        float s = sqrtf(trace + 1.0f);
        float t = 0.5f / s;

        q->q[3] = s * 0.5f;
        q->q[0] = (m->f[1][2] - m->f[2][1]) * t;
        q->q[1] = (m->f[2][0] - m->f[0][2]) * t;
        q->q[2] = (m->f[0][1] - m->f[1][0]) * t;
    }
    else
    {
        // Find largest diagonal element
        int i = 0;
        if (m->f[1][1] > m->f[0][0]) i = 1;
        if (m->f[2][2] > m->f[i][i]) i = 2;

        int j = s_next[i];
        int k = s_next[j];

        float s = sqrtf((m->f[i][i] - (m->f[j][j] + m->f[k][k])) + 1.0f);
        float t = 0.5f / s;

        float tmp[3];
        tmp[i] = s * 0.5f;
        tmp[j] = (m->f[j][i] + m->f[i][j]) * t;
        tmp[k] = (m->f[k][i] + m->f[i][k]) * t;

        q->q[0] = tmp[0];
        q->q[1] = tmp[1];
        q->q[2] = tmp[2];
        q->q[3] = (m->f[j][k] - m->f[k][j]) * t;
    }
}

void dlPackSTN(quaternion_u* q, const vec3_u* stan, const vec3_u* ttan, const vec3_u* norm)
{
    mtx_u m;

    // Copy identity matrix as base
    memcpy(&m, &g_identMtx, sizeof(mtx_u));

    float sx = stan->v[0], sy = stan->v[1], sz = stan->v[2];
    float tx = ttan->v[0], ty = ttan->v[1], tz = ttan->v[2];
    float nx = norm->v[0], ny = norm->v[1], nz = norm->v[2];

    // Store S vector in row 0
    m.f[0][0] = sx;
    m.f[0][1] = sy;
    m.f[0][2] = sz;

    // Store N vector in row 2
    m.f[2][0] = nx;
    m.f[2][1] = ny;
    m.f[2][2] = nz;

    // Compute handedness -- sign of the triple product (determinant)
    // cross(S,T) dotted with N
    float crossX = ty * sz - tz * sy;
    float crossY = tz * sx - tx * sz;
    float crossZ = tx * sy - ty * sx;
    float det = nx * crossX + ny * crossY + nz * crossZ;

    // flip = +1 if det >= 0, -1 if det < 0
    float flip = (det >= 0.0f) ? 1.0f : -1.0f;

    // Store T vector in row 1, scaled by handedness
    m.f[1][0] = tx * flip;
    m.f[1][1] = ty * flip;
    m.f[1][2] = tz * flip;

    // Convert rotation matrix to quaternion
    matrixToQuaternion(q, &m);

    // Ensure positive w component
    if (q->q[3] < 0.0f)
    {
        q->q[0] = -q->q[0];
        q->q[1] = -q->q[1];
        q->q[2] = -q->q[2];
        q->q[3] = -q->q[3];
    }

    // Avoid zero w
    if (q->q[3] == 0.0f)
        q->q[3] = 0.0000099999997f;

    // Scale w by handedness
    q->q[3] *= flip;
}

void dlCacheViewData(viewdef_s* view)
{
    dlviewcache.view = view;

    // Cache the 3 columns of the modelview matrix as direction vectors
    dlviewcache.dirx.v[0] = view->modelviewmtx.f[0][0];
    dlviewcache.dirx.v[1] = view->modelviewmtx.f[1][0];
    dlviewcache.dirx.v[2] = view->modelviewmtx.f[2][0];

    dlviewcache.diry.v[0] = view->modelviewmtx.f[0][1];
    dlviewcache.diry.v[1] = view->modelviewmtx.f[1][1];
    dlviewcache.diry.v[2] = view->modelviewmtx.f[2][1];

    dlviewcache.dirz.v[0] = view->modelviewmtx.f[0][2];
    dlviewcache.dirz.v[1] = view->modelviewmtx.f[1][2];
    dlviewcache.dirz.v[2] = view->modelviewmtx.f[2][2];

    // Pack STN (S=dirx, T=diry, N=dirz)
    dlPackSTN(&dlviewcache.stn, &dlviewcache.dirx, &dlviewcache.diry, &dlviewcache.dirz);

    // Compute near plane distance
    // n = dot(dirz, pos - dirz) - dot(pos, dirz)
    // which simplifies to: -dot(dirz, dirz) ... but game computes it explicitly
    float dx = view->pos.v[0] - dlviewcache.dirz.v[0];
    float dy = view->pos.v[1] - dlviewcache.dirz.v[1];
    float dz = view->pos.v[2] - dlviewcache.dirz.v[2];

    float dot1 = dlviewcache.dirz.v[0] * dx
        + dlviewcache.dirz.v[1] * dy
        + dlviewcache.dirz.v[2] * dz;

    float dot2 = view->pos.v[0] * dlviewcache.dirz.v[0]
        + view->pos.v[1] * dlviewcache.dirz.v[1]
        + view->pos.v[2] * dlviewcache.dirz.v[2];

    dlviewcache.n = dot1 - dot2;
}

void platformSetMatricesHW(mtx_u* perspmtx, mtx_u* modelviewmtx)
{
    if (modelviewmtx)
        memcpy(&mviewmtxcur, modelviewmtx, sizeof(mtx_u));

    if (perspmtx)
        memcpy(&perspmtxcur, perspmtx, sizeof(mtx_u));

    UpdateMVP();
}

void dlCopyBufferToTextureHW(int x1, int y1, int u1, int v1, int w, int h)
{
    unsigned int texHandle = s_toSend.values.texture[0].i[1];
    int texID = texGetTexID(texHandle);

    // Align dimensions to 8
    int alignedW = (w + 7) & ~7;
    int alignedH = (h + 7) & ~7;

    RECT srcRect;
    srcRect.left = x1;
    srcRect.top = y1;
    srcRect.right = x1 + alignedW;
    srcRect.bottom = y1 + alignedH;

    rStateWrite();

    if (texHandle == (unsigned int)-1)
        return;

    TexType type = textures[texHandle].type;
    if (type < TEX_TYPE_2D || (type > TEX_TYPE_RECTANGLE && type != TEX_TYPE_HDR))
        return;

    IDirect3DTexture9* destTex = s_pTexturesHW[texID].___u0.dxtex;
    if (!destTex)
        return;

    IDirect3DSurface9* srcSurface = nullptr;
    g_pd3dDevice->GetRenderTarget(0, &srcSurface);
    if (!srcSurface)
        return;

    IDirect3DSurface9* destSurface = nullptr;
    destTex->GetSurfaceLevel(0, &destSurface);
    if (!destSurface)
    {
        srcSurface->Release();
        return;
    }

    g_pd3dDevice->StretchRect(srcSurface, &srcRect, destSurface, &srcRect, D3DTEXF_NONE);

    destSurface->Release();
    srcSurface->Release();
}

void pbufferDeactivate(unsigned int deactivateFlags)
{
    // Build viewport and scissor from saved state
    D3DVIEWPORT9 vp;
    vp.X = s_toSend.values.viewport.i[0];
    vp.Y = s_toSend.values.viewport.i[1];
    vp.Width = s_toSend.values.viewport.i[2];
    vp.Height = s_toSend.values.viewport.i[3];
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;

    RECT scissor;
    scissor.left = s_toSend.values.scissor.i[0];
    scissor.top = s_toSend.values.scissor.i[1];
    scissor.right = s_toSend.values.scissor.i[0] + s_toSend.values.scissor.i[2];
    scissor.bottom = s_toSend.values.scissor.i[1] + s_toSend.values.scissor.i[3];

    if (sFrameBufferPrev != (unsigned int)-1)
    {
        PBuffer_s* v1 = &pbuffers[sFrameBufferPrev];
        PBufferHW_s* v2 = &s_pPBuffersHW[sFrameBufferPrev];

        int x1, y1, w, h;

        if (sFrameBufferPrev == DL_PBUFFER_FRAME)
        {
            // Clamp resolve region to pbuffer dimensions
            x1 = (int)(vp.X & ~7);
            y1 = (int)(vp.Y & ~7);
            int x2 = (int)((vp.Width + 7) & ~7);
            int y2 = (int)((vp.Height + 7) & ~7);

            if (v1->width < x1) x1 = v1->width;
            if (v1->height < y1) y1 = v1->height;

            int right = x1 + x2;
            int bottom = y1 + y2;
            if (v1->width < right)  right = v1->width;
            if (v1->height < bottom) bottom = v1->height;

            w = right - x1;
            h = bottom - y1;
        }
        else
        {
            x1 = 0;
            y1 = 0;
            w = v1->width;
            h = v1->height;
        }

        // Determine what needs resolving
        bool resolveColor = (deactivateFlags & 1) && (v1->colorTexture != (unsigned int)-1);
        bool resolveDepth = ((deactivateFlags >> 1) & 1) && (v1->depthTexture != (unsigned int)-1);

        v2->bColourResolvedOnLastDeactivate = 0;
        v2->bDepthResolvedOnLastDeactivate = 0;

        if (resolveColor)
        {
            if (v2->bColorRenderedAfterLastLastActivate)
            {
                texSelectTextureEx(v1->colorTexture, TEX_FUNC_MODULATE_RGBA);
                dlCopyBufferToTextureHW(x1, y1, x1, y1, w, h);
            }
            v2->bColourResolvedOnLastDeactivate = 1;
        }

        if (resolveDepth)
        {
            if (v2->bDepthRenderedAfterLastLastActivate)
            {
                texSelectTextureEx(v1->depthTexture, TEX_FUNC_MODULATE_RGBA);
                dlCopyBufferToTextureHW(x1, y1, x1, y1, w, h);
            }
            v2->bDepthResolvedOnLastDeactivate = 1;
        }

        //XBOX ONLY
        // Disable high precision blend for float buffers
        //if (v1->bFloat)
            //g_pd3dDevice->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, FALSE);
    }

    // Restore back buffer as render target
    g_pd3dDevice->SetRenderTarget(0, g_pD3dBackBufferTarget);
    g_pd3dDevice->SetDepthStencilSurface(g_pD3dDepthStencilTarget);
    g_pd3dDevice->SetViewport(&vp);
    g_pd3dDevice->SetScissorRect(&scissor);
}

void platformGetMatricesHW(mtx_u* perspmtx, mtx_u* modelviewmtx)
{
    if (modelviewmtx)
        memcpy(modelviewmtx, &mviewmtxcur, sizeof(mtx_u));

    if (perspmtx)
        memcpy(perspmtx, &perspmtxcur, sizeof(mtx_u));
}

void vec4mtx44mulvec3(vec4_u* v, const mtx_u* m, const vec3_u* v1)
{
    float x = v1->v[0];
    float y = v1->v[1];
    float z = v1->v[2];

    v->v[0] = m->f[0][0] * x + m->f[1][0] * y + m->f[2][0] * z + m->f[3][0];
    v->v[1] = m->f[0][1] * x + m->f[1][1] * y + m->f[2][1] * z + m->f[3][1];
    v->v[2] = m->f[0][2] * x + m->f[1][2] * y + m->f[2][2] * z + m->f[3][2];
    v->v[3] = m->f[0][3] * x + m->f[1][3] * y + m->f[2][3] * z + m->f[3][3];
}

BOOL platformProjectToScreen(vec3_u* outPos, const vec3_u* inPos)
{
    mtx_u perspmtx;
    mtx_u modelviewmtx;
    platformGetMatricesHW(&perspmtx, &modelviewmtx);

    mtx_u mvp;
    matrixMultiplyAligned(&mvp, &modelviewmtx, &perspmtx);

    vec4_u clip;
    vec4mtx44mulvec3(&clip, &mvp, inPos);

    float invW = 0.5f / clip.v[3];

    // Convert clip space to NDC then to screen [0,1] range
    outPos->v[0] = clip.v[0] * invW + 0.5f;  // x: left=0 right=1
    outPos->v[1] = -clip.v[1] * invW + 0.5f;  // y: flipped, top=0 bottom=1
    outPos->v[2] = clip.v[2] * invW + 0.5f;  // z: depth

    return clip.v[2] > 0.0f;  // true if in front of near plane
}

unsigned int sTextureCircleSun = 0u;

static inline unsigned char clampToByte(float v)
{
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    return (unsigned char)(v * 255.0f);
}

unsigned int texCreateTexture2DExCore(
    unsigned int th,
    unsigned int sizeX, int sizeY,
    int numChannels, unsigned __int8* data,
    unsigned int flags, int inMipCount, int checkSize)
{
    if (!data)
    {
        texFreeTexture(th);
        return (unsigned int)-1;
    }

    // Get texture entry
    texdef_s* tex = (th < 0x1000) ? &textures[th] : nullptr;
    if (!tex)
    {
        texFreeTexture(th);
        return (unsigned int)-1;
    }

    // Set texture metadata
    tex->width = sizeX;
    tex->height = sizeY;
    tex->lightmapScale = 1.0f;
    tex->type = TEX_TYPE_2D;
    tex->flags = (tex->flags | flags) | 2;

    // Select D3D format based on channel count
    D3DFORMAT fmt;
    switch (numChannels)
    {
    case 1:  fmt = D3DFMT_L8;       break;
    case 3:  fmt = D3DFMT_X8R8G8B8; break;
    case 4:  fmt = D3DFMT_A8R8G8B8; break;
    default:
        texFreeTexture(th);
        return (unsigned int)-1;
    }

    // Convert RGB to XRGB if needed
    unsigned __int8* uploadData = data;
    std::vector<unsigned __int8> converted;

    if (numChannels == 3)
    {
        // Expand RGB to XRGB
        int pixels = sizeX * sizeY;
        converted.resize(pixels * 4);
        for (int i = 0; i < pixels; i++)
        {
            converted[i * 4 + 0] = data[i * 3 + 2]; // B
            converted[i * 4 + 1] = data[i * 3 + 1]; // G
            converted[i * 4 + 2] = data[i * 3 + 0]; // R
            converted[i * 4 + 3] = 0xFF;             // X
        }
        uploadData = converted.data();
        fmt = D3DFMT_X8R8G8B8;
    }
    else if (numChannels == 4)
    {
        // Convert RGBA to ARGB
        int pixels = sizeX * sizeY;
        converted.resize(pixels * 4);
        for (int i = 0; i < pixels; i++)
        {
            converted[i * 4 + 0] = data[i * 4 + 2]; // B
            converted[i * 4 + 1] = data[i * 4 + 1]; // G
            converted[i * 4 + 2] = data[i * 4 + 0]; // R
            converted[i * 4 + 3] = data[i * 4 + 3]; // A
        }
        uploadData = converted.data();
    }

    // Find free HW slot
    int hwSlot = -1;
    for (int i = 0; i < 0x1001; i++)
    {
        if (!s_pTexturesHWUsed[i])
        {
            s_pTexturesHWUsed[i] = 1;
            hwSlot = i;
            break;
        }
    }

    if (hwSlot == -1)
    {
        texFreeTexture(th);
        return (unsigned int)-1;
    }

    tex->hw[0] = hwSlot;

    // Create D3D9 texture
    IDirect3DTexture9* d3dTex = nullptr;
    HRESULT hr = g_pd3dDevice->CreateTexture(
        sizeX, sizeY,
        1,              // mip levels
        0,              // usage
        fmt,
        D3DPOOL_MANAGED,
        &d3dTex,
        nullptr);

    if (FAILED(hr) || !d3dTex)
    {
        s_pTexturesHWUsed[hwSlot] = 0;
        tex->hw[0] = (unsigned int)-1;
        texFreeTexture(th);
        return (unsigned int)-1;
    }

    // Upload texture data
    D3DLOCKED_RECT locked;
    if (SUCCEEDED(d3dTex->LockRect(0, &locked, nullptr, 0)))
    {
        int srcPitch = sizeX * (numChannels == 1 ? 1 : 4);
        for (int row = 0; row < sizeY; row++)
        {
            memcpy(
                (unsigned __int8*)locked.pBits + row * locked.Pitch,
                uploadData + row * srcPitch,
                srcPitch);
        }
        d3dTex->UnlockRect(0);
    }

    // Store in HW table
    s_pTexturesHW[hwSlot].___u0.dxtex = d3dTex;
    s_pTexturesHW[hwSlot].texHandle = th;
    s_pTexturesHW[hwSlot].texLevel = 0;
    s_pTexturesHW[hwSlot].maxMipmapLevel = 0;

    return th;
}

unsigned int texgenCreateCircleMap(
    int size, int channels,
    float radius, float gradientOffset,
    const float* col1, const float* col2,
    unsigned int flags)
{
    int dataSize = size * size * channels;
    if (dataSize <= 0)
        return (unsigned int)-1;

    unsigned char* data = (unsigned char*)memAllocAlignCore(
        dataSize, 0, 0, "source/texture/texgen.c", 295, nullptr, 1);
    if (!data)
        return (unsigned int)-1;

    int   half = size / 2;
    float halfF = (float)(half - 1);

    for (int y = 0; y < size; y++)
    {
        float fy = (float)(y - half) / halfF;

        for (int x = 0; x < size; x++)
        {
            float fx = (float)(x - half) / halfF;
            float dist = sqrtf(fx * fx + fy * fy);
            int   idx = (y * size + x) * channels;

            if (dist > radius)
            {
                // Outside radius -- use col2
                for (int c = 0; c < channels; c++)
                    data[idx + c] = clampToByte(col2[c]);
            }
            else if (dist >= gradientOffset)
            {
                // Gradient region -- lerp from col1 to col2
                float t = (dist - gradientOffset) / (radius - gradientOffset);
                for (int c = 0; c < channels; c++)
                    data[idx + c] = clampToByte(col1[c] + (col2[c] - col1[c]) * t);
            }
            else
            {
                // Inside gradient offset -- use col1
                for (int c = 0; c < channels; c++)
                    data[idx + c] = clampToByte(col1[c]);
            }
        }
    }

    unsigned int handle = texCreateHandle();
    unsigned int result = (unsigned int)-1;

    if (handle != (unsigned int)-1)
        result = texCreateTexture2DExCore(handle, size, size, channels, data, flags, 0, 0);

    memFreeFlags((char*)data, 1);
    return result;
}

void sunflareGetSunTexture(viewdef_s* view)
{
    if (sTextureCircleSun != (unsigned int)-1)
        return;

    // Try to load texture from scene descriptor
    int sceneDescriptorIndex = view->sceneDescriptorIndex;
    const char* sunFlareName = s_sceneDescriptorArray[sceneDescriptorIndex].m_sceneDescriptor.skyCubeMapSunFlareName;

    if (sunFlareName && *sunFlareName)
    {
        unsigned int tex = texLoadTextureName(sunFlareName, 0);
        sTextureCircleSun = tex;
        if (tex != (unsigned int)-1)
            return;
    }

    // Fall back to procedurally generated circle map
    float col1[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float col2[6] = { 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

    sTextureCircleSun = texgenCreateCircleMap(64, 4, 1.0f, 0.75f, col1, col2, 0);
}

void dlTextureRectangleZ(
    float x1, float y1,
    float x2, float y2,
    float z,
    float u1, float v1,
    float u2, float v2,
    float cr, float cg, float cb, float ca)
{
    idlContextStruct.data = &idlContextData;
    idlContextStruct.ptr = nullptr;

    // type 22 = position + texcoord + colour
    float* p = dlBeginMain(&idlContextStruct, 4, 4, 22);
    idlContextStruct.ptr = p;
    if (!p) return;

    // Vertex layout per vertex: u, v, cr, cg, cb, ca, x, y, z
    // Bottom-left
    *idlContextStruct.ptr++ = u1;
    *idlContextStruct.ptr++ = v1;
    *idlContextStruct.ptr++ = cr;
    *idlContextStruct.ptr++ = cg;
    *idlContextStruct.ptr++ = cb;
    *idlContextStruct.ptr++ = ca;
    *idlContextStruct.ptr++ = x1;
    *idlContextStruct.ptr++ = y1;
    *idlContextStruct.ptr++ = z;

    // Bottom-right
    *idlContextStruct.ptr++ = u2;
    *idlContextStruct.ptr++ = v1;
    *idlContextStruct.ptr++ = cr;
    *idlContextStruct.ptr++ = cg;
    *idlContextStruct.ptr++ = cb;
    *idlContextStruct.ptr++ = ca;
    *idlContextStruct.ptr++ = x2;
    *idlContextStruct.ptr++ = y1;
    *idlContextStruct.ptr++ = z;

    // Top-left
    *idlContextStruct.ptr++ = u1;
    *idlContextStruct.ptr++ = v2;
    *idlContextStruct.ptr++ = cr;
    *idlContextStruct.ptr++ = cg;
    *idlContextStruct.ptr++ = cb;
    *idlContextStruct.ptr++ = ca;
    *idlContextStruct.ptr++ = x1;
    *idlContextStruct.ptr++ = y2;
    *idlContextStruct.ptr++ = z;

    // Top-right
    *idlContextStruct.ptr++ = u2;
    *idlContextStruct.ptr++ = v2;
    *idlContextStruct.ptr++ = cr;
    *idlContextStruct.ptr++ = cg;
    *idlContextStruct.ptr++ = cb;
    *idlContextStruct.ptr++ = ca;
    *idlContextStruct.ptr++ = x2;
    *idlContextStruct.ptr++ = y2;
    *idlContextStruct.ptr++ = z;

    dlEndMain(idlContextStruct.data, idlContextStruct.ptr);
}

void sunflareGfxSun(
    float readIntensity,
    float sx1, float sy1,
    float sx2, float sy2,
    float sunz,
    const vec3_u* col,
    unsigned int sunTex)
{
    // Calculate slight expansion of the sun quad (10% border)
    float padX = fabsf(sx2 - sx1) * 0.1f;
    float padY = fabsf(sy2 - sy1) * 0.1f;

    texSelectTextureEx(sTextureCircleSun, TEX_FUNC_MODULATE_RGBA);
    platformBlendHW(4);
    platformDepthTestHW(1);
    platformDepthFuncHW(21);

    // Set shader colour from col vector
    //shaderVarSetTexture(sShaderVarTexture0, sunTex);
    //s_constColour.v[0] = col->v[0];
    //s_constColour.v[1] = col->v[1];
    //s_constColour.v[2] = col->v[2];
    //s_constColour.v[3] = 1.0f;
    //shaderEnableIteration(sShaderCopyCompressed);

    // Draw sun quad with slight padding and full UV coverage
    dlTextureRectangleZ(
        sx1 - padX,     // x1
        sy1 - padY,     // y1
        sx2 + padX,     // x2
        sy2 + padY,     // y2
        sunz,           // z
        0.0f,           // u1
        0.0f,           // v1
        1.0f,           // u2
        1.0f,           // v2
        col->v[0],      // cr
        col->v[1],      // cg
        col->v[2],      // cb
        1.0f);          // ca

    g_shaderEnabledVP = 0;
    g_shaderEnabledFP = 0;
    platformDepthTestHW(0);
    platformBlendHW(0);
}

void sunflareGfx(viewdef_s* view)
{
    int sceneDescriptorIndex = view->sceneDescriptorIndex;

    // Calculate sun screen size based on viewport
    float winW = windowfullscreen.width;
    float winH = windowfullscreen.height;
    float pixW = (float)(int)(view->window.width * winW);
    float pixH = (float)(int)(view->window.height * winH);
    float winLeft = view->window.left * 63.0f + 0.5f;
    float winRight = view->window.right * 63.0f + 0.5f;
    float winTop = view->window.top * 63.0f + 0.5f;
    float winBot = view->window.bottom * 63.0f + 0.5f;

    float sizeX = ((winRight - winLeft) / pixW) * (winW / pixW) * 0.5f;
    float sizeY = ((winBot - winTop) / pixH) * 0.5f;

    // Push render state
    if (s_topIndex + 1 < 16)
    {
        ++s_topIndex;
        ++s_top;
        g_rState = &s_top->values;
    }

    // Project sun position to screen
    stackPush(&mviewmtxstack, (char*)&mviewmtxcur);
    stackPush(&perspmtxstack, (char*)&perspmtxcur);
    platformSetMatricesHW(&view->perspmtx, &view->modelviewmtx);

    vec3_u screenPos;
    platformProjectToScreen(
        &screenPos,
        &s_sceneDescriptorArray[sceneDescriptorIndex].m_sceneDescriptor.flareSunPos);

    platformPopMatricesHW();
    rStatePop();

    // Normalize sun direction
    vec3_u sunDir;
    vecnormaliseGold(&sunDir,
        &s_sceneDescriptorArray[sceneDescriptorIndex].m_sceneDescriptor.flareSunPos);

    // Calculate screen bounds of sun quad
    float sx1 = screenPos.v[0] - sizeX;
    float sy1 = screenPos.v[1] - sizeY;
    float sx2 = screenPos.v[0] + sizeX;
    float sy2 = screenPos.v[1] + sizeY;

    // Dot product of view dir with sun dir (visibility check)
    float dot = view->dir.v[0] * sunDir.v[0]
        + view->dir.v[1] * sunDir.v[1]
        + view->dir.v[2] * sunDir.v[2];

    // Get sun colour from volume data
    sceneVolumeDataValidate(sceneDescriptorIndex);
    float sunR = s_volumeData[sceneDescriptorIndex].sunRgb[0];
    float sunG = s_volumeData[sceneDescriptorIndex].sunRgb[1];
    float sunB = s_volumeData[sceneDescriptorIndex].sunRgb[2];

    // Visibility culling
    if (sx2 <= 0.0f || sx1 >= 1.0f || sy2 <= 0.0f || sy1 >= 1.0f || dot <= 0.0f)
        return;

    float sunIntensity = s_volumeData[sceneDescriptorIndex].sunIntensity;

    vec3_u sunColor;
    sunColor.v[0] = sunIntensity * sunR;
    sunColor.v[1] = sunIntensity * sunG;
    sunColor.v[2] = sunIntensity * sunB;

    dlPush2D();
    sunflareGetSunTexture(view);

    if (!rdebugflags.disableSunDecal)
    {
        float sunZ = screenPos.v[2];
        if (sunZ > 1.0f) sunZ = 1.0f;

        sunflareGfxSun(1.0f, sx1, sy1, sx2, sy2, sunZ, &sunColor, sTextureCircleSun);
    }

    platformPopMatricesHW();
    rStatePop();
}

void camCalcEnvLight(int cubeSide, unsigned int envLightHandle)
{
    // Copy current framebuffer into env texture slot
    texSelectTextureEx((unsigned int)-1, TEX_FUNC_MODULATE_RGBA);
    dlCopyBufferToTextureHW(0, 0, 0, 0, 128, 128);
    texSelectTextureEx((unsigned int)-1, TEX_FUNC_MODULATE_RGBA);

    // Generate mip chain by repeatedly downsampling
    int mipScale = 2;
    int mipSize = 64;

    do
    {
        dlPush2D();

        float invScale = 1.0f / (float)mipScale;
        float invHalf = 1.0f / (float)(mipScale >> 1);

        camDrawFullScreenQuadDebug(
            0.0f,     // offsetX
            0.0f,     // offsetY
            invHalf,  // maxX
            invHalf,  // maxY
            invScale, // maxU
            invScale);// maxV

        platformPopMatricesHW();
        rStatePop();

        // Clear texture state
        rStateValueStore* v = g_rState;
        rStateInt3* tex = &v->texture[0];
        tex->i[0] = 0;
        tex->i[1] = -1;
        tex->i[2] = 0;
        tex->base.writeCB.writeInt3 = writeTextureEx;

        if (tex->base.link.next == (slinkdef_s*)v->texture)
        {
            tex->base.link.next = s_top->changed.head;
            s_top->changed.head = &tex->base.link;
        }

        tex->base.vtype = 3;
        tex->base.numValues = 3;
        setState(&tex->base, 468, 3);

        // Copy downsampled mip into correct position
        dlCopyBufferToTextureHW(0, 129 - mipSize, 0, 0, mipSize, mipSize);

        mipScale *= 2;
        mipSize = 128 / mipScale;
    } while (mipSize >= 1);

    // Final texture state reset
    rStateValueStore* v = g_rState;
    rStateInt3* tex = &v->texture[0];
    tex->i[0] = 0;
    tex->i[1] = -1;
    tex->i[2] = 0;
    tex->base.writeCB.writeInt3 = writeTextureEx;

    if (tex->base.link.next == (slinkdef_s*)v->texture)
    {
        tex->base.link.next = s_top->changed.head;
        s_top->changed.head = &tex->base.link;
    }

    tex->base.vtype = 3;
    tex->base.numValues = 3;
    setState(&tex->base, 468, 3);
}

int quarterSceneBufferState = 0;
unsigned __int8 sHdrEnabled = 0u;
unsigned int quarterSceneBuffer = 4294967295u;
float s_hdrGaussianTot = 0.0;

unsigned int texgenCreateGaussianMap(int xmax, int ymax, int channels, float stddev, float mean)
{
    const float SCALE = 0.071428575f; // 1/14
    const float SIGMA = 2.3548f;      // 2*sqrt(2*ln(2))
    const float INV_HALF = 0.5f;

    unsigned char* data = (unsigned char*)memAllocAlignCore(
        0x1000, 0, 0, "source/texture/texgen.c", 181, nullptr, 1);

    if (!data)
        return (unsigned int)-1;

    s_hdrGaussianTot = 0.0f;

    unsigned char* p = data + 2;

    for (int y = 0; y < 32; y++)
    {
        float fy = (float)(y - 16) * SCALE;
        float fy2 = fy * fy;

        for (int x = 0; x < 32; x++)
        {
            float fx = (float)(x - 16) * SCALE;
            float dist = sqrtf(fx * fx + fy2) - mean;
            float t = dist * SIGMA;
            float val = expf(-(t * t * INV_HALF));

            // Zero out border pixels
            if (y == 0 || x == 0 || y == 31 || x == 31)
                val = 0.0f;

            s_hdrGaussianTot += val;

            unsigned char b = (unsigned char)(val * 255.0f);
            *(p - 2) = b;
            *(p - 1) = b;
            *(p + 0) = b;
            *(p + 1) = b;
            p += 4;
        }
    }

    unsigned int handle = texCreateHandle();
    unsigned int result = (unsigned int)-1;

    if (handle != (unsigned int)-1)
        result = texCreateTexture2DExCore(handle, 32, 32, 4, data, 0x16000, 0, 0);

    memFreeFlags((char*)data, 1);
    return result;
}

void filterUtilDownSampleGaussianCore(
    unsigned int pbufferToRender,
    unsigned int pbufferRenderTarget,
    const float* avgSampleOffsets,
    int arraycount,
    ShaderTreeIteration* shader,
    float u1, float v1,
    float u2, float v2)
{
    // Get render target dimensions
    int width = 0;
    int height = 0;
    if (pbufferRenderTarget < 0x40)
    {
        PBuffer_s* pb = &pbuffers[pbufferRenderTarget];
        if (pb->valid)
        {
            width = pb->width;
            height = pb->height;
        }
    }

    // Save matrix state
    stackPush(&mviewmtxstack, (char*)&mviewmtxcur);
    stackPush(&perspmtxstack, (char*)&perspmtxcur);

    // Push render state
    if (s_topIndex + 1 < 16)
    {
        ++s_topIndex;
        ++s_top;
        g_rState = &s_top->values;
    }

    // Switch to render target
    unsigned int prevFrameBuffer = sFrameBuffer;
    if (sFrameBuffer != pbufferRenderTarget)
    {
        sFrameBufferPrev = sFrameBuffer;
        sFrameBuffer = pbufferRenderTarget;
        pbufferDeactivate(3);
        //if (pbufferRenderTarget != (unsigned int)-1)
            //pbufferActivate(pbufferRenderTarget, 0);
    }

    dlPush2D();
    platformBlendHW(0);
    platformViewportGL(0, 0, width, height);

    // Get source texture
    unsigned int colorTexture = (unsigned int)-1;
    if (pbufferToRender < 0x40)
    {
        PBuffer_s* pb = &pbuffers[pbufferToRender];
        if (pb->valid)
            colorTexture = pb->colorTexture;
    }

    //shaderVarSetTexture(s_vars_0[0], colorTexture);

    if (avgSampleOffsets)
        //shaderVarSetFloat2Array(s_vars_0[1], avgSampleOffsets, 16);

    if (shader)
        //shaderEnableIteration(shader);
   /* else*/
    {
        g_shaderEnabledVP = 0;
        g_shaderEnabledFP = 0;
    }

    // Draw full screen quad
    dlTextureRectangleZ(
        1.0f,  // x1
        0.0f,  // y1
        0.0f,  // x2
        1.0f,  // y2
        0.0f,  // z
        u2,    // u1
        v2,    // v1
        u1,    // u2
        v1,    // v2
        1.0f, 1.0f, 1.0f, 1.0f); // cr, cg, cb, ca

    // Restore full viewport
    platformViewportGL(0, 0, sFrameSizeX, sFrameSizeY);
    platformPopMatricesHW();
    rStatePop();

    // Restore previous frame buffer
    if (sFrameBuffer != prevFrameBuffer)
    {
        sFrameBufferPrev = sFrameBuffer;
        sFrameBuffer = prevFrameBuffer;
        pbufferDeactivate(3);
        if (prevFrameBuffer != (unsigned int)-1)
            pbufferActivate(prevFrameBuffer, 3);
    }

    rStatePop();
    platformPopMatricesHW();
}

void filterUtilDownSampleGaussianBlur4x4(
    unsigned int source,
    unsigned int destination,
    float u1, float v1,
    float u2, float v2,
    unsigned int intermediate)
{
    // Get source pbuffer dimensions
    float srcWidth = 0.0f;
    float srcHeight = 0.0f;

    if (source < 0x40)
    {
        PBuffer_s* pb = &pbuffers[source];
        if (pb->valid)
        {
            srcWidth = (float)pb->width;
            srcHeight = (float)pb->height;
        }
    }

    float invWidth = (srcWidth > 0.0f) ? (1.0f / srcWidth) : 0.0f;
    float invHeight = (srcHeight > 0.0f) ? (1.0f / srcHeight) : 0.0f;

    // Build 4x sample offset array
    // Layout per entry: [v_offset, u_offset0, v_offset, u_offset1, v_offset, u_offset2, v_offset, u_offset3]
    float avgSampleOffsets[4 * 8];
    float* p = avgSampleOffsets;

    for (int i = 0; i < 4; i++)
    {
        float vOff = (float)i * invHeight;

        *(p - 1 + 1) = invWidth * 0.0f;          // u offset 0
        p[1] = invWidth;                   // u offset 1
        p[3] = invWidth * 2.0f;            // u offset 2
        p[5] = invWidth * c_PhantomMovementExpansionRatio; // u offset 3

        p[0] = vOff;  // v offset 0
        p[2] = vOff;  // v offset 1
        p[4] = vOff;  // v offset 2
        p[6] = vOff;  // v offset 3

        p += 8;
    }

    filterUtilDownSampleGaussianCore(
        source,
        destination,
        avgSampleOffsets,
        16,
        s_shaders_0[1],
        u1, v1, u2, v2);
}

void filterUtilQuarterScale(
    viewdef_s* view,
    unsigned int source,
    unsigned int intermediate,
    unsigned int destination)
{
    // Get source pbuffer width
    float srcWidth = 0.0f;
    if (DL_PBUFFER_FRAME < 0x40)
    {
        PBuffer_s* pb = &pbuffers[DL_PBUFFER_FRAME];
        if (pb->valid)
            srcWidth = (float)pb->width;
    }

    // Get source pbuffer height
    float srcHeight = 0.0f;
    if (DL_PBUFFER_FRAME < 0x40)
    {
        PBuffer_s* pb = &pbuffers[DL_PBUFFER_FRAME];
        if (pb->valid)
            srcHeight = (float)pb->height;
    }

    float invWidth = (srcWidth > 0.0f) ? (2.0f / srcWidth) : 0.0f;
    float invHeight = (srcHeight > 0.0f) ? (2.0f / srcHeight) : 0.0f;

    filterUtilDownSampleGaussianBlur4x4(
        source,
        destination,
        view->window.left,                    // u1
        view->window.bottom - invHeight,       // v1
        view->window.right - invWidth,        // u2
        view->window.top,                      // v2
        intermediate);
}

unsigned int dlSetFrameBuffer(unsigned int dst, unsigned int deactivateFlags, unsigned int activateFlags)
{
    unsigned int prev = sFrameBuffer;

    if (sFrameBuffer != dst)
    {
        sFrameBufferPrev = sFrameBuffer;
        sFrameBuffer = dst;
        pbufferDeactivate(deactivateFlags);

        if (dst != (unsigned int)-1)
            pbufferActivate(dst, activateFlags);
    }

    return prev;
}

void hdrResolve(viewdef_s* view)
{
    // Create gaussian map if not yet created
    if (s_hdrGaussian == (unsigned int)-1)
    {
        s_hdrGaussian = texgenCreateGaussianMap(32, 32, 4, 1.0f, 0.0f);
        texSelectTextureEx((unsigned int)-1, TEX_FUNC_MODULATE_RGBA);
    }

    quarterSceneBufferState = 0;

    if (!sHdrEnabled || (view->flags & 0x20) != 0)
        return;

    // Push render state
    if (s_topIndex + 1 < 16)
    {
        ++s_topIndex;
        ++s_top;
        g_rState = &s_top->values;
    }

    platformColourMaskHW(1, 1, 1, 1);
    platformDepthWriteHW(0);
    platformBlendHW(0);

    if (rdebugflags.simpleHdr)
    {
        // Simple HDR path -- just blit frame texture
        dlPush2D();
        platformBlendHW(0);
        platformDepthTestHW(0);

        int x = (int)(view->window.left * windowfullscreen.width);
        int y = (int)(view->window.top * windowfullscreen.height);
        int w = (int)(view->window.right * windowfullscreen.width) - x;
        int h = (int)(view->window.bottom * windowfullscreen.height) - y;

        platformViewportGL(x, y, w, h);

        texSelectTextureEx(DL_TEXTURE_FRAME, TEX_FUNC_MODULATE_RGBA);

        dlTextureRectangleZ(
            1.0f,              // x1
            0.0f,              // y1
            0.0f,              // x2
            1.0f,              // y2
            0.0f,              // z
            view->window.right,// u1
            view->window.top,  // v1
            view->window.left, // u2
            1.0f,              // v2
            1.0f, 1.0f, 1.0f, 1.0f); // cr, cg, cb, ca

        platformPopMatricesHW();
        rStatePop();
        rStatePop();
    }
    else
    {
        // Full HDR path
        // Determine view quadrant
        int viewNum;
        if (view->window.top != 0.0f)
        {
            viewNum = (view->window.left != 0.0f) ? 3 : 2;
        }
        else
        {
            viewNum = (view->window.left != 0.0f) ? 1 : 0;
        }

        // Deactivate current framebuffer
        if (sFrameBuffer != (unsigned int)-1)
        {
            sFrameBufferPrev = sFrameBuffer;
            sFrameBuffer = (unsigned int)-1;
            pbufferDeactivate(1);
        }

        filterUtilQuarterScale(view, DL_PBUFFER_FRAME, DL_PBUFFER_FRAME_SCRATCH, quarterSceneBuffer);
        quarterSceneBufferState = 1;

        //hdrMeasureLuminance();
        //hdrCalculateAdaptiveLumninace(viewNum);
        //hdrBrightPass(viewNum);
        //hdrBrightBlur();

        dlSetFrameBuffer(DL_PBUFFER_FRAME, 0, 0);
        //hdrToneAndBloomFilter(view, viewNum);

        g_shaderEnabledVP = 0;
        g_shaderEnabledFP = 0;
        rStatePop();
    }
}

void dlEndViewHdr(viewdef_s* view)
{
    if (view->flags & 0x20)
    {
        int envMapSide = view->envMapSide;

        if (rdebugflags.envMapDebugGFX)
        {
            int tex = DL_TEXTURE_ENV[envMapSide];
            if (tex != -1)
            {
                texSelectTextureEx(tex, TEX_FUNC_MODULATE_RGBA);
                dlCopyBufferToTextureHW(0, 0, 0, 0, 128, 128);
            }
        }

        camCalcEnvLight(envMapSide, (unsigned int)-1);

        view->envMapSide = (view->envMapSide + 1) % 6;
    }

    hdrResolve(view);
}

void dlCopyLastFrame(viewdef_s* view)
{
    float winW = (float)windowfullscreen.width;
    float winH = (float)windowfullscreen.height;

    // Convert normalised window coords to pixel coords
    int x1 = (int)(view->window.left * winW + 0.5f);
    int y1 = (int)(view->window.top * winH + 0.5f);
    int w = (int)((view->window.right - view->window.left) * winW + 0.5f);
    int h = (int)((view->window.bottom - view->window.top) * winH + 0.5f);

    // Clamp to screen edges
    if (x1 + w > (int)winW) w = (int)winW - x1;
    if (y1 + h > (int)winH) h = (int)winH - y1;

    dlPush2D();
    platformColourMaskHW(1, 1, 1, 0);  // RGB only, no alpha
    platformDepthWriteHW(0);

    texSelectTextureEx(DL_TEXTURE_FRAME_LAST, TEX_FUNC_MODULATE_RGBA);
    dlCopyBufferToTextureHW(x1, y1, x1, y1, w, h);

    platformPopMatricesHW();
    rStatePop();
}

void sceneGFXEx(viewdef_s* view, unsigned __int8 envMap)
{
    // Determine if we should render the scene
    bool renderScene = true;

    //if (s_vmCmd.active && !s_vmCmd.renderGame)
    //    renderScene = false;
    //else if (!s_renderScene)
    //    renderScene = false;

    unsigned __int8 v8 = (unsigned __int8)renderScene;

    // Clear shader state
    //shState.passparams = 0;
    //shState.shaderparams = 0;
    //shState.info = 0;
    //shState.pass = 0;
    //shState.material = 0;
    //shState.iteration = 0;
    //shState.definitions = 0;
    //shState.solution = 0;

    if (renderScene)
    {
        // Find and render visible props
        opaqueProp* props[1024];
        //int numProps = sceneFindVisibleEx(props, 1024, view, &view->frustum);
        //propGFX(props, numProps, view);

        if (!(view->flags & 0x20))
        {
            // Render flares
            unsigned __int8 normalMap = 0;
            int flareCount = 0;
            //for (int i = 0; i < 32 && flareCount < s_flareNum; i++)
            //{
            //    flare_s* f = &s_flares[i];
            //    if (!f->used) continue;
            //    if (!normalMap && f->normalMapped)
            //        normalMap = flareGenerateNormalMap(view);
            //    flareGfxRenderFlare(view, f);
            //    flareCount++;
            //}

            // Sun flare
            if (rdebugflags.drawsky &&
                s_sceneDescriptorArray[view->sceneDescriptorIndex].m_sceneDescriptor.flareOn)
                sunflareGfx(view);

            //filterViewGfxPreHdr(view);
            //trackfxGfx();

            // Clouds
            //if (rdebugflags.drawclouds && s_bEnable_1 && s_bInitialised_0)
            //{
            //    cloudAreaGFXArea(&s_cloudArea, view);
            //    if (s_bEnable_2 && s_cloudLayer.m_bEnable)
            //        cloudLayerRenderLayer(&s_cloudLayer, view);
            //}

            //laserBeamGfx(view);
            //lightningGfx(view);
            //particleGFX(view);
            //weatherParticleGfx(view);
            //rankGlows(view);

            // Glows
            //unsigned __int8 numGlows = s_numDrawGlows;
            //for (int i = 0; i < numGlows; i++)
            //{
            //    if (s_drawGlows[i] != (unsigned int)-1)
            //    {
            //        glowRenderGlow(view, s_drawGlows[i]);
            //        numGlows = s_numDrawGlows;
            //    }
            //}

            // Deferred volume swings
            /*while (s_DeferredRenderedVolumeSwingsFirst)
            {
                VolumeSwingElem_s* j = s_DeferredRenderedVolumeSwingsFirst;
                if (j == s_DeferredRenderedVolumeSwingsLast)
                    s_DeferredRenderedVolumeSwingsLast = nullptr;

                s_DeferredRenderedVolumeSwingsFirst = j->pNext;

                volumeSwingRender(
                    &view->pos,
                    &j->data.prevFromPos,
                    &j->data.prevToPos,
                    &j->data.fromPos,
                    &j->data.toPos,
                    j->data.fRadius,
                    j->data.middleStartOffset,
                    j->data.middleEndOffset,
                    j->data.tex,
                    &j->data.color,
                    &j->data.endSwing,
                    j->data.fromAlpha,
                    j->data.swingTesselation);

                memFreeFlags((char*)j, 1);
            }*/

            // Deferred volume lines
            float lineColor[4] = { 0.0f, 1.0f, 1.0f, 0.0f };
            /*while (s_DeferredRenderedVolumeLinesFirst)
            {
                VolumeLineElem_s* v = s_DeferredRenderedVolumeLinesFirst;
                if (v == s_DeferredRenderedVolumeLinesLast)
                    s_DeferredRenderedVolumeLinesLast = nullptr;

                s_DeferredRenderedVolumeLinesFirst = v->pNext;

                volumeLineRenderInternal(
                    &v->data.from,
                    &v->data.to,
                    v->data.fRadius,
                    v->data.fFakeVolumeIncrease,
                    1.0f,
                    v->data.texHandle,
                    &v->data.color,
                    1,
                    1,
                    DL_BLEND_ADDRGB);

                memFreeFlags((char*)v, 1);
            }*/

            //mistGfx(view);
            //electricGfx(view);
            //lightrayGfx(view);
            //weatherGfx(view);

            //if (s_GameEffectGfxCallback)
            //    s_GameEffectGfxCallback(view);

            //dustGfx(view);
            //spaceDustGfx(view);
        }

        if (v8)
            dlEndViewHdr(view);
    }

    // Explosions + post HDR
    //if (!(view->flags & 0x20) && v8)
    //{
    //    unsigned __int8 alreadyRendered = 0;
    //    int expCount = 0;
    //    for (int k = 0; k < 16 && expCount < s_explosionNum; k++)
    //    {
    //        if (!s_explosions[k].used) continue;
    //        if (explosionGfxRenderExplosion(view, &s_explosions[k], alreadyRendered))
    //            alreadyRendered = 1;
    //        expCount++;
    //    }
    //    filterViewGfxPostHdr(view);
    //}

    dlCopyLastFrame(view);

    // Transparent overlay
   /* unsigned int flags = view->flags;
    if (!(flags & 0x20) && s_transparentOverlayGfxCallback && (flags & 2))
    {
        if (!v8)
            dlSetFrameBuffer(DL_PBUFFER_FRAME, 1, 0);
        g_shaderEnabledVP = 0;
        g_shaderEnabledFP = 0;
        s_transparentOverlayGfxCallback(view);
    }*/

    // Deactivate frame buffer
    if (sFrameBuffer != (unsigned int)-1)
    {
        sFrameBufferPrev = sFrameBuffer;
        sFrameBuffer = (unsigned int)-1;
        pbufferDeactivate(1);
    }
}

float(__cdecl* s_camSkyBlendCallback)(void*, void*) = NULL;
void* s_camSkyBlendUserData = NULL;
float s_skyBlend = 0.0;
void camRenderView(viewdef_s* view)
{
    camCalculateView(view);

    if (s_camSkyBlendCallback)
        s_skyBlend = s_camSkyBlendCallback(view, s_camSkyBlendUserData);

    dlCacheViewData(view);

    // Calculate viewport from normalized window coords
    int x = (int)(view->window.left * windowfullscreen.width);
    int y = (int)(view->window.top * windowfullscreen.height);
    int w = (int)(view->window.right * windowfullscreen.width) - x;
    int h = (int)(view->window.bottom * windowfullscreen.height) - y;

    platformViewportGL(x, y, w, h);
    platformSetMatricesHW(&view->perspmtx, &view->modelviewmtx);
    sceneGFXEx(view, 0);
}

void camDrawFullScreenQuadDebug(
    float offsetX, float offsetY,
    float maxX, float maxY,
    float maxU, float maxV)
{
    idlContextStruct.ptr = nullptr;
    idlContextStruct.data = &idlContextData;

    s_dl.constvtx.col = platformColour(1.0f, 1.0f, 1.0f, 1.0f);
    s_dl.constusage |= 1;

    float* v = dlBeginMain(&idlContextStruct, 6, 4, 18);
    idlContextStruct.ptr = v;
    if (!v) return;

    // Vertex layout: x, y, u, v (type 18 = position + texcoord)
    // Bottom-left
    *idlContextStruct.ptr++ = offsetX;
    *idlContextStruct.ptr++ = offsetY;
    *idlContextStruct.ptr++ = 0.0f;
    *idlContextStruct.ptr++ = 0.0f;

    // Bottom-right
    *idlContextStruct.ptr++ = offsetX + maxX;
    *idlContextStruct.ptr++ = offsetY;
    *idlContextStruct.ptr++ = maxU;
    *idlContextStruct.ptr++ = 0.0f;

    // Top-right
    *idlContextStruct.ptr++ = offsetX + maxX;
    *idlContextStruct.ptr++ = maxY;
    *idlContextStruct.ptr++ = maxU;
    *idlContextStruct.ptr++ = maxV;

    // Top-left
    *idlContextStruct.ptr++ = offsetX;
    *idlContextStruct.ptr++ = maxY;
    *idlContextStruct.ptr++ = 0.0f;
    *idlContextStruct.ptr++ = maxV;

    dlEndMain(idlContextStruct.data, idlContextStruct.ptr);
}

void camRenderEnvMapDebug()
{
    const float SIZE = 128.0f;
    int slot = 0;

    // Loop through all env map textures
    for (unsigned int* texPtr = DL_TEXTURE_ENV;
        (int)texPtr < (int)&s_envLightDownSampleTex;
        texPtr++, slot++)
    {
        unsigned int texHandle = *texPtr;
        if (texHandle == (unsigned int)-1)
            continue;

        // Set texture state
        rStateValueStore* v3 = g_rState;
        rStateInt3* texture = &v3->texture[0];

        texture->i[0] = 0;
        texture->i[1] = texHandle;
        texture->i[2] = 0;
        texture->base.writeCB.writeInt3 = writeTextureEx;

        if (texture->base.link.next == (slinkdef_s*)v3->texture)
        {
            texture->base.link.next = s_top->changed.head;
            s_top->changed.head = &texture->base.link;
        }

        texture->base.vtype = 3;
        texture->base.numValues = 3;
        setState(&texture->base, 468, 3);

        dlPush2D();

        float w = (float)windowfullscreen.width;
        float h = (float)windowfullscreen.height;

        float offsetX = (float)slot * (SIZE / w);
        float maxX = SIZE / w;
        float maxY = SIZE / h;

        camDrawFullScreenQuadDebug(
            offsetX,    // offsetX
            0.0f,       // offsetY
            maxX,       // maxX
            maxY,       // maxY
            1.0f,       // maxU
            1.0f);      // maxV

        platformPopMatricesHW();
        rStatePop();
    }

    // Reset texture state to -1
    rStateValueStore* v9 = g_rState;
    rStateInt3* v10 = &v9->texture[0];

    v10->i[0] = 0;
    v10->i[1] = -1;
    v10->i[2] = 0;
    v10->base.writeCB.writeInt3 = writeTextureEx;

    if (v10->base.link.next == (slinkdef_s*)v9->texture)
    {
        v10->base.link.next = s_top->changed.head;
        s_top->changed.head = &v10->base.link;
    }

    v10->base.vtype = 3;
    v10->base.numValues = 3;
    setState(&v10->base, 468, 3);
}

void camRenderViewsWithFlag(unsigned int viewFlag, slinklistdef_s* list)
{
    slinklistdef_s* head = (slinklistdef_s*)viewListAll.head;

    if (&viewListAll == head)
    {
        s_currentview = nullptr;
        return;
    }

    unsigned int v3 = viewFlag | 0x10;

    while (true)
    {
        viewdef_s* v4 = (viewdef_s*)((char*)head + viewListAll.offset);

        if ((v4->flags & v3) == v3)
        {
            s_currentview = v4;

            // Call pre-render callback
            if (v4->callback)
                v4->callback(0, v4);

            if (v4->flags & 8)
            {
                sceneDescriptorPrepareForRender(v4->sceneDescriptorIndex, v4);

                if (!(v4->flags & 0x20))
                {
                    camRenderView(v4);
                    if (rdebugflags.envMapDebugGFX)
                        camRenderEnvMapDebug();
                }

                // Sun light invalidation check
                unsigned int m_sunLight = s_sceneDescriptorArray[v4->sceneDescriptorIndex].m_sunLight;
                if (m_sunLight != (unsigned int)-1)
                {
                    unsigned int handle = worldlights[m_sunLight].handle;
                    if (handle)
                    {
                        if (worldlights[handle >> 20].handle == handle)
                            worldlights[m_sunLight].flags &= ~1u;
                        else
                            worldlights[m_sunLight].handle = 0;
                    }
                }

                s_renderingUsingSceneDescriptorIndex = -1;
            }

            // Call post-render callback
            if (v4->callback)
                v4->callback(1, v4);
        }

        head = (slinklistdef_s*)head->head;
        if (&viewListAll == head)
        {
            s_currentview = nullptr;
            return;
        }
    }
}

void __fastcall camRenderViewsAfterCallback(slinklistdef_s* list)
{
    unsigned int v1; // r28
    slinklistdef_s* i; // r11
    int v3; // r10
    unsigned int v4; // r31
    unsigned int v5; // r11
    slinklistdef_s* head; // r31
    void(__fastcall * v7)(int); // r11

    v1 = 0;
    for (i = (slinklistdef_s*)viewListAll.head; &viewListAll != i; i = (slinklistdef_s*)i->head)
    {
        v3 = *(int*)((char*)&i->offset + viewListAll.offset);
        if ((v3 & 0x12) == 0x12 && (v3 & 8) != 0 && (v3 & 0x20) == 0)
            ++v1;
    }
    v4 = DL_PBUFFER_FRAME;
    v5 = sFrameBuffer;
    if (sFrameBuffer != DL_PBUFFER_FRAME)
    {
        sFrameBufferPrev = sFrameBuffer;
        sFrameBuffer = DL_PBUFFER_FRAME;
        pbufferDeactivate(0);
        if (v4 != -1)
            pbufferActivate(v4, v1 > 1);
        v5 = sFrameBuffer;
    }
    head = (slinklistdef_s*)viewListAll.head;
    if (&viewListAll != (slinklistdef_s*)viewListAll.head)
    {
        do
        {
            if ((*(int*)((BYTE*)&head->offset + viewListAll.offset) & 0x10) != 0)
            {
                v7 = *(void(__fastcall**)(int))((char*)&head[155].head + viewListAll.offset);
                s_currentview = (viewdef_s*)((char*)head + viewListAll.offset);
                if (v7)
                    v7(2);
            }
            head = (slinklistdef_s*)head->head;
        } while (&viewListAll != head);
        v5 = sFrameBuffer;
    }
    if (v5 != -1)
    {
        sFrameBufferPrev = v5;
        sFrameBuffer = -1;
        pbufferDeactivate(0);
    }
    s_currentview = 0;
}

void camRenderAllViews()
{
    ++timerRenderFrameNum;

    if (initData.graphicsEnable)
    {
        dlSetFrameBuffers();
        stackPush(&mviewmtxstack, (char*)&mviewmtxcur);
        stackPush(&perspmtxstack, (char*)&perspmtxcur);

        if (s_topIndex + 1 < 16)
        {
            ++s_topIndex;
            ++s_top;
            g_rState = &s_top->values;
        }
    }

    camRenderViewsWithFlag(4, &viewListAll);
    camRenderViewsWithFlag(2, &viewListAll);
    camRenderViewsAfterCallback(&viewListAll);

    platformViewportGL(0, 0, (int)windowfullscreen.width, (int)windowfullscreen.height);

    //if (s_gameFinishRenderingCallback)
    //    s_gameFinishRenderingCallback();

    fontRender();

    // Advance debug draw buffer
    s_bufferContentsAreValid = 1;
    debugdrawBuffer_s* v1 = &ddBuffer[activeBuffer];
    unsigned int v2 = ((DWORD*)&v1->regs)[7];

    v1->regs.colour[0] = 1.0f;
    v1->regs.colour[1] = 1.0f;
    v1->regs.colour[2] = 1.0f;
    v1->regs.colour[3] = 1.0f;
    v1->regs.command.command = 0;
    v1->regs.blendMode = dd_BlendOff;
    v1->regs.pointCount = 0;
    v1->bufferCur = v1->bufferBase;
    ((DWORD*)&v1->regs)[7] = v2 | 0x80000000;
    currentBuffer = v1;

    if (initData.graphicsEnable)
    {
        rStatePop();
        platformPopMatricesHW();
    }
}

void platformPushMatricesHW()
{
    stackPush(&mviewmtxstack, (char*)&mviewmtxcur);
    stackPush(&perspmtxstack, (char*)&perspmtxcur);
}

void pbufferActivate(unsigned int handle, unsigned int activateFlags)
{
    bool clearBuffer = (activateFlags >> 2) & 1;
    bool resolveColor = activateFlags & 1;
    bool resolveDepth = (activateFlags >> 1) & 1;

    if (handle >= 0x40)
        return;

    PBuffer_s* pb = &pbuffers[handle];
    PBufferHW_s* hw = &s_pPBuffersHW[handle];

    if (!pb->valid)
        return;

    // Set render target
    g_pd3dDevice->SetRenderTarget(0, hw->rendertarget);
    g_pd3dDevice->SetDepthStencilSurface(hw->renderdepthstencil);

    // Build viewport and scissor from saved state
    D3DVIEWPORT9 vp;
    vp.X = s_toSend.values.viewport.i[0];
    vp.Y = s_toSend.values.viewport.i[1];
    vp.Width = s_toSend.values.viewport.i[2];
    vp.Height = s_toSend.values.viewport.i[3];
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;

    RECT scissor;
    scissor.left = s_toSend.values.scissor.i[0];
    scissor.top = s_toSend.values.scissor.i[1];
    scissor.right = s_toSend.values.scissor.i[0] + s_toSend.values.scissor.i[2];
    scissor.bottom = s_toSend.values.scissor.i[1] + s_toSend.values.scissor.i[3];

    g_pd3dDevice->SetViewport(&vp);
    g_pd3dDevice->SetScissorRect(&scissor);

    rStateBlock* stk = s_top;

    if (clearBuffer)
    {
        platformViewportGL(0, 0, pb->width, pb->height);
        platformClearColourHW(0.0f, 0.0f, 0.0f, 0.0f);
        platformClearDepthHW(1.0f);

        // Set face cull state
        rStateValueStore* v9 = g_rState;
        v9->facecull.i[0] = 3;
        v9->facecull.base.writeCB.writeInt1 = writeFaceCull;

        if (v9->facecull.base.link.next == (slinkdef_s*)v9)
        {
            v9->facecull.base.link.next = stk->changed.head;
            stk->changed.head = &v9->facecull.base.link;
        }

        v9->facecull.base.vtype = 1;
        v9->facecull.base.numValues = 1;
        setState(&v9->facecull.base, 0, 1);

        platformDepthFuncHW(21);
        platformDepthTestHW(1);
        platformShadeHW(28);
        platformBlendHW(0);
        texSelectTextureEx((unsigned int)-1, TEX_FUNC_MODULATE_RGBA);
    }

    bool doColor = resolveColor && (pb->colorTexture != (unsigned int)-1);
    bool doDepth = resolveDepth && (pb->depthTexture != (unsigned int)-1);

    if (doColor || doDepth)
    {
        // Push render state
        if (s_topIndex + 1 < 16)
        {
            ++s_topIndex;
            ++s_top;
            g_rState = &s_top->values;
        }

        platformPushMatricesHW();
        platformSetMatrices2DHW();
        platformStencilTestHW(0);
        platformBlendHW(0);
        platformAlphaTestHW(0, 0);

        // Set face cull (disable backface culling for full screen quad)
        rStateValueStore* v16 = g_rState;
        v16->facecull.i[0] = s_toSend.values.facecull.i[0] & ~1;
        v16->facecull.base.writeCB.writeInt1 = writeFaceCull;

        if (v16->facecull.base.link.next == (slinkdef_s*)v16)
        {
            v16->facecull.base.link.next = s_top->changed.head;
            s_top->changed.head = &v16->facecull.base.link;
        }

        v16->facecull.base.vtype = 1;
        v16->facecull.base.numValues = 1;
        setState(&v16->facecull.base, 0, 1);

        platformViewportGL(0, 0, pb->width, pb->height);
        platformScissorGL(0, 0, pb->width, pb->height);

        // Color texture
        /*if (doColor)
        {
            platformColourMaskHW(1, 1, 1, 1);
            shaderVarSetTexture(s_colourTexture, pb->colorTexture);

            if (s_useColourTex->value != 1)
            {
                int n = s_useColourTex->numIterations;
                for (int i = 0; i < n; i++)
                    s_useColourTex->iterations[i].iteration->currentState |= s_useColourTex->iterations[i].stateVarMask;
                s_useColourTex->value = 1;
            }
        }
        else
        {
            platformColourMaskHW(0, 0, 0, 0);

            if (s_useColourTex->value)
            {
                int n = s_useColourTex->numIterations;
                for (int i = 0; i < n; i++)
                    s_useColourTex->iterations[i].iteration->currentState &= ~s_useColourTex->iterations[i].stateVarMask;
                s_useColourTex->value = 0;
            }
        }*/

        // Depth texture
        //if (doDepth)
        //{
        //    //shaderVarSetTexture(s_depthTexture, pb->depthTexture);

        //    if (s_useDepthTex->value != 1)
        //    {
        //        int n = s_useDepthTex->numIterations;
        //        for (int i = 0; i < n; i++)
        //            s_useDepthTex->iterations[i].iteration->currentState |= s_useDepthTex->iterations[i].stateVarMask;
        //        s_useDepthTex->value = 1;
        //    }

        //    platformDepthFuncHW(26);
        //    platformDepthWriteHW(1);
        //    platformDepthTestHW(1);
        //}
        //else
        //{
        //    platformDepthWriteHW(0);
        //    platformDepthTestHW(0);

        //    if (s_useDepthTex->value)
        //    {
        //        int n = s_useDepthTex->numIterations;
        //        for (int i = 0; i < n; i++)
        //            s_useDepthTex->iterations[i].iteration->currentState &= ~s_useDepthTex->iterations[i].stateVarMask;
        //        s_useDepthTex->value = 0;
        //    }
        //}

        // Draw full screen quad to resolve textures
        //g_storedProgFP = lastprog_p;
        //g_storedProgVP = lastprog_v;
        //shaderEnableIteration(s_writePbuffer);

        dlTextureRectangleZ(
            0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f);

        g_shaderEnabledVP = 0;
        g_shaderEnabledFP = 0;

        //shaderSetVS(g_storedProgVP);
        //shaderSetPS(g_storedProgFP);

        platformPopMatricesHW();
        rStatePop();

        if (pb->depthTexture != (unsigned int)-1)
            platformDepthFuncHW(21);
    }

    // Reset render tracking flags
    hw->bColorRenderedAfterLastLastActivate = 0;
    hw->bDepthRenderedAfterLastLastActivate = 0;

    // Xbox only -- no-op on PC
    // if (pb->bFloat)
    //     D3DDevice_SetRenderState_HighPrecisionBlendEnable(g_pd3dDevice, 1);
}

void debugdrawInitialise(int BufferSize)
{
    ddBuffer[0].bufferSize = BufferSize;

    unsigned __int8* v2 = nullptr;
    if (BufferSize > 0)
        v2 = (unsigned __int8*)memAllocAlignCore(
            BufferSize, 0, 0, __FILE__, 690, nullptr, 1);

    currentBuffer = ddBuffer;

    ddBuffer[0].bufferBase = v2;
    ddBuffer[0].bufferCur = v2;
    ddBuffer[0].bufferMax = v2 + ddBuffer[0].bufferSize;

    ddBuffer[0].regs.colour[0] = 1.0f;
    ddBuffer[0].regs.colour[1] = 1.0f;
    ddBuffer[0].regs.colour[2] = 1.0f;
    ddBuffer[0].regs.colour[3] = 1.0f;

    ddBuffer[0].regs.command.command = 0;
    ddBuffer[0].regs.blendMode = dd_BlendOff;
    ddBuffer[0].regs.pointCount = 0;

    // Set high bit of regs dword 7
    ((DWORD*)&ddBuffer[0].regs)[7] |= 0x80000000;

    activeBuffer = 0;
}

void idrawPolygon2D(const vec4_u* colour, int numpoints, vec2_u* points)
{
    idlContextStruct.data = &idlContextData;
    idlContextStruct.ptr = nullptr;

    texSelectTextureEx((unsigned int)-1, TEX_FUNC_MODULATE_RGBA);

    if (colour && idlContextStruct.data == &idlContextData)
    {
        s_dl.constvtx.col = platformColour(colour->v[0], colour->v[1], colour->v[2], colour->v[3]);
        s_dl.constusage |= 1;
    }

    // type=0 (position only XY), 6 vertices (quad as 2 triangles), primitiveType=4 (triangle list)
    float* p = dlBeginMain(&idlContextStruct, 4, 6, 0);
    idlContextStruct.ptr = p;
    if (!p) return;

    // Quad split into 2 triangles: [0,1,2] and [0,2,3]
    // points[0] = top-left
    // points[1] = top-right  
    // points[2] = bottom-right
    // points[3] = bottom-left

    // Triangle 1: 0, 1, 2
    *idlContextStruct.ptr++ = points[0].v[0];
    *idlContextStruct.ptr++ = points[0].v[1];
    *idlContextStruct.ptr++ = points[1].v[0];
    *idlContextStruct.ptr++ = points[1].v[1];
    *idlContextStruct.ptr++ = points[2].v[0];
    *idlContextStruct.ptr++ = points[2].v[1];

    // Triangle 2: 0, 2, 3
    *idlContextStruct.ptr++ = points[0].v[0];
    *idlContextStruct.ptr++ = points[0].v[1];
    *idlContextStruct.ptr++ = points[2].v[0];
    *idlContextStruct.ptr++ = points[2].v[1];
    *idlContextStruct.ptr++ = points[3].v[0];
    *idlContextStruct.ptr++ = points[3].v[1];

    dlEndMain(idlContextStruct.data, idlContextStruct.ptr);
}

void idrawQuad2D(const vec4_u* colour, float x, float y, float w, float h)
{
    idlContextStruct.data = &idlContextData;
    idlContextStruct.ptr = nullptr;

    texSelectTextureEx((unsigned int)-1, TEX_FUNC_MODULATE_RGBA);

    if (colour && idlContextStruct.data == &idlContextData)
    {
        s_dl.constvtx.col = platformColour(colour->v[0], colour->v[1], colour->v[2], colour->v[3]);
        s_dl.constusage |= 1;
    }

    // type=0 (XY only), 4 vertices, primitiveType=4 (triangle strip)
    float* p = dlBeginMain(&idlContextStruct, 4, 4, 0);
    idlContextStruct.ptr = p;
    if (!p) return;

    // Triangle strip quad: TL, TR, BL, BR
    *idlContextStruct.ptr++ = x;
    *idlContextStruct.ptr++ = y;
    *idlContextStruct.ptr++ = x + w;
    *idlContextStruct.ptr++ = y;
    *idlContextStruct.ptr++ = x;
    *idlContextStruct.ptr++ = y + h;
    *idlContextStruct.ptr++ = x + w;
    *idlContextStruct.ptr++ = y + h;

    dlEndMain(idlContextStruct.data, idlContextStruct.ptr);
}

void fontPreRenderHW(unsigned char resetMatrix)
{
    if (!initData.graphicsEnable)
        return;

    // Push render state
    if (s_topIndex + 1 < 16)
    {
        ++s_topIndex;
        ++s_top;
        g_rState = &s_top->values;
    }

    stackPush(&mviewmtxstack, (char*)&mviewmtxcur);
    stackPush(&perspmtxstack, (char*)&perspmtxcur);

    if (resetMatrix)
        platformSetMatrices2DHW();

    // Disable backface culling
    rStateValueStore* v = g_rState;
    v->facecull.i[0] = s_toSend.values.facecull.i[0] & ~1;
    v->facecull.base.writeCB.writeInt1 = writeFaceCull;

    if (v->facecull.base.link.next == (slinkdef_s*)v)
    {
        v->facecull.base.link.next = s_top->changed.head;
        s_top->changed.head = &v->facecull.base.link;
    }

    v->facecull.base.vtype = 1;
    v->facecull.base.numValues = 1;
    setState(&v->facecull.base, 0, 1);

    platformDepthTestHW(0);
    platformDepthWriteHW(0);
    platformBlendHW(2);
}