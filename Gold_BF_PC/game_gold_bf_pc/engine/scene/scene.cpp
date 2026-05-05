#include "scene.h"
#include "engine/string/engstring.h"
#include "engine/mem.h"
#include "framework/template/CTemplate.h"
#include "engine/platform_pc/hw.h"
#include "engine/engineinit.h"

CStrPool* s_keyPool = NULL;
CStrPool* s_strValuePool = NULL;

CPerViewSceneDescriptorSettings s_sceneDescriptorArray[3];
LightBase worldlights[512];
CMgdList<sceneDescriptor_s, CStdGetHandle<sceneDescriptor_s>, 11> s_sceneDescriptors;
SceneVolumeData_s s_volumeData[4];
struct CML 
{
    vec3_u v[6];
};

enum SKYMODE : __int32
{
    SKY_MODE_NONE = 0x0,
    SKY_MODE_FLAT = 0x1,
    SKY_MODE_BOX = 0x2,
    SKY_MODE_BLEND_BOX = 0x3,
    SKY_MODE_BOX_NON_CUBED = 0x4,
    SKY_MODE_DOME = 0x5,
    SKY_MODE_OB = 0x6,
};

struct opaqueProp* s_skyModelProps[4][2];
SKYMODE s_skyMode = SKY_MODE_NONE;
CML s_spaceCML;
int s_renderingUsingSceneDescriptorIndex = -1;
void sceneDescriptorInitialise()
{
    s_sceneDescriptorArray[0].m_used = 1;

    // Allocate and init key pool
    void* keyMem = memAllocAlignCore(0xA4C, 0, 0, __FILE__, 589, "memalloc no group", 2);
    if (keyMem)
        s_keyPool = new(keyMem) CHashStrPool(-1);
    else
        s_keyPool = nullptr;

    // Allocate and init string value pool
    void* strMem = memAllocAlignCore(0xA4C, 0, 0, __FILE__, 589, "memalloc no group", 2);
    if (strMem)
        s_strValuePool = new(strMem) CHashStrPool(-1);
    else
        s_strValuePool = nullptr;

    // Init scene descriptor list
    s_sceneDescriptors.Init(0, 25);

    // Clear sky model props and space CML
    memset(&s_spaceCML, 0, 0x48);

    // Set space CML default values
    s_spaceCML.v[3].v[0] = 0.0020000001f;
    s_spaceCML.v[3].v[1] = 0.0040000002f;
    s_spaceCML.v[3].v[2] = 0.039999999f;

    memset(s_skyModelProps, 0, sizeof(s_skyModelProps));
}

float flt_82CBA124 = 0.0;
float s_sunSizeY = 0.0;
void skyInitialise()
{
    s_skyMode = SKY_MODE_NONE;

    int width = (int)windowfullscreen.width;
    int height = (int)windowfullscreen.height;

    flt_82CBA124 = 64.0f / (float)width;
    s_sunSizeY = 64.0f / (float)height;
}

unsigned int s_hackPreallocatedTexBuffer = 0xFFFFFFFF;
unsigned int s_bEnable_1 = 0u;
unsigned int s_bInitialised_0 = 0u;
unsigned int s_bEnable = 1u;
int s_cloudAreaMemGroup = 0;
unsigned int s_noiseOctaveTex[2] = { 0u, 0u };
unsigned int s_bInitialised_2 = 0u;
int s_cloudVolumeMemGroup = 0;
int s_sliceTesselation = 0;
int s_sliceCount = 0;
int s_firstSliceNum = 0;
int s_lastSliceNum = 0;
unsigned int s_densityTex = 4294967295u;
unsigned int s_framebufferPB = 4294967295u;
unsigned int s_framebufferColorTex = 4294967295u;
unsigned int s_framebufferDepthTex = 4294967295u;
unsigned int s_impostorsColorTex = 4294967295u;
unsigned int s_impostorsPB = 4294967295u;
unsigned int s_volumeSlicesVB = 4294967295u;
unsigned int s_volumeSlicesIB = 4294967295u;
float* s_pVolumeSlicesVertexBufferData = NULL;
unsigned __int16* s_pVolumeSlicesIndiceBufferData = NULL;
unsigned int s_bInitialised_1 = 0u;

void generateVolumeSlices(
    int sliceCount,
    int sliceTesselation,
    unsigned int vertexBufferHandle,
    float** pVertexBufferDataPtr,
    unsigned int indiceBufferHandle,
    unsigned short** pIndiceBufferDataPtr)
{
    unsigned int vbHandle = s_volumeSlicesVB;
    unsigned int ibHandle = s_volumeSlicesIB;
    int tess = s_sliceTesselation;
    int slices = s_sliceCount;

    // Lock vertex buffer
    float* vbData = nullptr;
    if (vbHandle)
    {
        s_pDlVertexBuffers[vbHandle].m_vb->Lock(0, 0, (void**)&vbData, 0);
    }
    s_pVolumeSlicesVertexBufferData = vbData;

    // Lock index buffer
    unsigned short* ibData = nullptr;
    if (ibHandle)
    {
        s_pDlIndexBuffers[ibHandle].m_ib->Lock(0, 0, (void**)&ibData, 0);
    }
    s_pVolumeSlicesIndiceBufferData = ibData;

    float* vp = vbData;

    // Generate vertices
    if (slices > 0)
    {
        float sliceMaxF = (float)(slices - 1);

        for (int s = 0; s < slices; s++)
        {
            float z = ((float)s * 2.0f) / sliceMaxF - 1.0f;

            if (tess > 0)
            {
                float tessMaxF = (float)(tess - 1);

                for (int y = 0; y < tess; y++)
                {
                    float yf = ((float)y * 2.0f) / tessMaxF - 1.0f;

                    for (int x = 0; x < tess; x++)
                    {
                        float xf = ((float)x * 2.0f) / tessMaxF - 1.0f;
                        vp[0] = xf;
                        vp[1] = yf; // stored at offset +4
                        vp[2] = z;  // stored at offset +8
                        vp += 3;
                    }
                }
            }
        }

        // Generate indices
        unsigned short* ip = ibData;
        int rowBase = 0;

        for (int s = 0; s < slices; s++)
        {
            for (int y = 0; y < tess - 1; y++)
            {
                for (int x = 0; x < tess - 1; x++)
                {
                    unsigned short tl = (unsigned short)(rowBase + y * tess + x);
                    unsigned short tr = (unsigned short)(rowBase + y * tess + x + 1);
                    unsigned short bl = (unsigned short)(rowBase + (y + 1) * tess + x);
                    unsigned short br = (unsigned short)(rowBase + (y + 1) * tess + x + 1);

                    // Triangle 1
                    *ip++ = tl;
                    *ip++ = tr;
                    *ip++ = bl;

                    // Triangle 2
                    *ip++ = bl;
                    *ip++ = tr;
                    *ip++ = br;
                }
            }
            rowBase += tess * tess;
        }
    }

    // Unlock buffers
    if (vbHandle)
        s_pDlVertexBuffers[vbHandle].m_vb->Unlock();
    if (ibHandle)
        s_pDlIndexBuffers[ibHandle].m_ib->Unlock();
}

int dword_82CC4C20 = 0; // weak
int dword_82CC4BB8[] = { 0 }; // weak
int dword_82CC4BBC = 0; // weak
int dword_82CC4BC0 = 0; // weak
int dword_82CC4BC4 = 0; // weak
float s_impostorsInvalidateCosAngle = 0.0;

vec4_u bgCol = { { 0.2, 0.2, 0.2, 0.5 } };

struct SImpostorTexCoords_s
{
    vec2_u topLeft;
    vec2_u bottomRight;
};

struct SImpostorElement_s
{
    int level;
    unsigned int bValid;
    SImpostorTexCoords_s texCoords;
    SImpostorElement_s* pNext;
};

SImpostorElement_s* s_impostorsAvailableByLevel[4] = { NULL, NULL, NULL, NULL };

void initImpostors()
{
    // Initialize impostor counts per level
    dword_82CC4BB8[0] = 4;
    dword_82CC4BBC = 16;
    dword_82CC4BC0 = 64;
    dword_82CC4BC4 = 256;
    dword_82CC4C20 = 340;

    // cos(1 degree in radians) for invalidation angle
    s_impostorsInvalidateCosAngle = cosf(0.01745329238474369f);

    float bgColW = bgCol.__s1.w; // bgCol+0xC = w component

    int levelIndex = 0;
    int v1 = 0;

    do
    {
        int count = dword_82CC4BB8[v1];
        int gridSize = (int)sqrtf((float)count);
        float cellSize = bgColW / (float)gridSize;

        // Allocate impostor array for this level
        SImpostorElement_s* impostors = nullptr;
        if (count * 28 > 0)
            impostors = (SImpostorElement_s*)memAllocAlignCore(
                count * 28, s_cloudVolumeMemGroup, 0,
                "source/terrain/cloudVolume.c", 802, nullptr, 1);

        s_impostorsAvailableByLevel[v1] = impostors;

        if (count > 0 && impostors)
        {
            // Grid origin based on level index
            int gridX = levelIndex / 2;
            int gridY = levelIndex % 2;
            float originX = (float)gridX * bgColW * 0.5f;
            float originY = (float)gridY * bgColW * 0.5f;

            SImpostorElement_s* cur = impostors;
            for (int i = 0; i < count; i++)
            {
                cur->level = levelIndex;

                int row = i / gridSize;
                int col = i % gridSize;

                float u = (float)row * cellSize + originX;
                float v = (float)col * cellSize + originY;

                cur->texCoords.topLeft.v[1] = u;
                cur->texCoords.topLeft.v[0] = v;
                cur->texCoords.bottomRight.v[1] = u + cellSize;
                cur->texCoords.bottomRight.v[0] = v + cellSize;

                // Link to next, last wraps to null
                SImpostorElement_s* next = cur + 1;
                if (i >= count - 1)
                    next = nullptr;
                cur->pNext = next;

                cur++;
            }
        }

        v1++;
        levelIndex++;
    } while (v1 < 4);
}

void cloudVolumeInitialise()
{
    if (s_cloudVolumeMemGroup == -1)
    {
        unsigned int v0 = memAllocGroupRandomBlockSizeEx("cloudVolumes", 0x32000, 0);
        s_cloudVolumeMemGroup = v0;

        memgroupdef_s* v1 = nullptr;
        if (v0 != (unsigned int)-1 && (int)v0 >= 0 && v0 < 0x40)
        {
            if (memgroups[v0].used)
                v1 = &memgroups[v0];
        }

        if (v1)
            v1->assertonfail = 1;
    }

    int v2 = s_sliceTesselation;
    int v3 = s_sliceCount;

    s_firstSliceNum = 0;
    s_lastSliceNum = s_sliceCount - 1;
    s_densityTex = -1;
    s_framebufferPB = -1;
    s_framebufferColorTex = -1;
    s_framebufferDepthTex = -1;
    s_impostorsPB = -1;
    s_impostorsColorTex = -1;

    s_volumeSlicesVB = dlVBCreate(v2 * v2 * v3, 1);
    s_volumeSlicesIB = dlIBCreate(((v2 - 1) * (v2 - 1) * v3) * 3 * 2 * sizeof(int));

    generateVolumeSlices(
        s_sliceCount,
        s_sliceTesselation,
        s_volumeSlicesVB,
        &s_pVolumeSlicesVertexBufferData,
        s_volumeSlicesIB,
        &s_pVolumeSlicesIndiceBufferData);

    initImpostors();
    s_bInitialised_1 = 1;
}

void cloudAreaInitialise()
{
    if (!s_bEnable)
        return;

    if (s_cloudAreaMemGroup == -1)
    {
        unsigned int v1 = memAllocGroupRandomBlockSizeEx("cloudArea", 0x32000, 0);
        s_cloudAreaMemGroup = v1;

        memgroupdef_s* v2 = nullptr;
        if (v1 != (unsigned int)-1 && (int)v1 >= 0 && v1 < 0x40)
        {
            if (memgroups[v1].used)
                v2 = &memgroups[v1];
        }

        if (v2)
            v2->assertonfail = 1;
    }

    s_noiseOctaveTex[0] = -1;
    s_noiseOctaveTex[1] = -1;

    cloudVolumeInitialise();
    s_bInitialised_2 = 1;
}

struct SCloudParticle_s
{
    vec3_u center;
    vec3_u scale;
    float radius;
};

struct spheredef_s
{
    vec3_u center;
    float radius;
    aabbdef_u aabb;
};

struct SCloud_s
{
    unsigned int particlesCount;
    SCloudParticle_s* pFirstParticle;
    unsigned int bVisible;
    float cameraToCloudDist;
    spheredef_s boundingSphere;
    vec3_u scale;
    aabbdef_u axisAlignedBoundingBox;
    vec3_u lastCloudToCameraDir;
    vec3_u lastCameraPos;
    vec3_u lastModelMtxUpDir;
    vec3_u lastImpostorScale;
    SImpostorElement_s* pImpostor;
};

struct SCloudElement_s
{
    SCloud_s* pCloud;
    SCloudElement_s* pNextCloudElement;
    SCloudElement_s* pPrevCloudElement;
};

struct SCloudArea_s
{
    unsigned int m_bEnable;
    unsigned int m_bInitialised;
    aabbdef_u m_cloudAreaAABB;
    vec3_u m_cloudSizeMin;
    vec3_u m_cloudSizeMax;
    SCloudElement_s* m_pFirstCloudElement;
    int m_cloudCount;
    unsigned int m_bNoIntersectingClouds;
    float m_cloudSharpness;
    float m_noiseOctaveWeights[8];
    float m_noiseOctaveNormWeights[8];
    float m_noiseOctaveEvolFrequencies[8];
    float m_noiseOctaveBlendsTemp[8];
    float m_noiseOctaveBlends[8];
    float m_noiseTexScaleTiling;
    float m_noiseInfluence;
    float m_lightScattering;
    float m_cloudMaxLighting;
    float m_cloudMinLighting;
    float m_rayStepLength;
    vec3_u m_lightColor;
    vec3_u m_darkColor;
};

SCloudArea_s s_cloudArea;

void cloudAreaInitArea(SCloudArea_s* pCloudArea)
{
    if (!s_bEnable)
        return;

    s_cloudArea.m_bEnable = 1;
    s_cloudArea.m_pFirstCloudElement = nullptr;
    memset(s_cloudArea.m_noiseOctaveBlends, 0, 0x20);
    s_cloudArea.m_bInitialised = 1;
}

unsigned int s_bEnable_2 = 0u;
unsigned int s_noiseTextures[2] = { 0u, 0u };
int dword_82CC4D98 = 0;
unsigned int s_densityTex_0 = 0u;
unsigned int s_curvedPlaneVB = 4294967295u;
unsigned int s_curvedPlaneIB = 4294967295u;
unsigned int s_bInitialised_3 = 0u;
float s_curvedPlaneRadius = 0.0;
float* s_curvedPlaneVBDataPtr = NULL;
unsigned __int16* s_curvedPlaneIBDataPtr = NULL;

void generateCurvedPlane()
{
    const int   GRID_SIZE = 33; // 33x33 grid = 1089 vertices
    const float SCALE = 62.5f;
    const float UV_SCALE = 0.03125f; // 1/32
    const float DISP_SCALE = 0.00390625f; // 1/256

    float radius = s_curvedPlaneRadius;
    float radiusSq = radius * radius;

    // Build vertex data on stack
    float vertData[1089 * 6]; // 6 floats per vertex
    float* vp = vertData;

    for (int y = -16; y <= 16; y++)
    {
        float fy = (float)y * SCALE;
        float fyUV = (float)(y + 16) * UV_SCALE;
        float fySq = fy * fy;

        for (int x = -16; x <= 16; x++)
        {
            float fx = (float)x * SCALE;
            float fxUV = (float)(x + 16) * UV_SCALE;
            float distSq = (fx * fx + fySq) * DISP_SCALE;
            float dist = (distSq < 1.0f) ? sqrtf(distSq) : 1.0f;

            float zOffset = sqrtf(radiusSq - fy * fy - fx * fx) - radius;

            vp[0] = fxUV;          // u
            vp[1] = fyUV;          // v
            vp[2] = 1.0f - dist;   // opacity/blend
            vp[3] = fy;            // world y
            vp[4] = zOffset;       // z displacement
            vp[5] = fx;            // world x
            vp += 6;
        }
    }

    // Build index data on stack
    unsigned short idxData[0x1080 / 2];
    unsigned short* ip = idxData;
    int v13 = 0;

    while (v13 < 1056)
    {
        for (int i = 33; i > 0; i--)
        {
            *ip++ = (unsigned short)(v13);
            *ip++ = (unsigned short)(v13 + 33);
            v13++;
        }
    }

    // Upload vertex data
    float* vbData = nullptr;
    if (s_curvedPlaneVB)
        s_pDlVertexBuffers[s_curvedPlaneVB].m_vb->Lock(0, 0, (void**)&vbData, 0);

    s_curvedPlaneVBDataPtr = vbData;
    if (vbData)
        memcpy(vbData, vertData, sizeof(vertData));

    if (s_curvedPlaneVB)
        s_pDlVertexBuffers[s_curvedPlaneVB].m_vb->Unlock();

    // Upload index data
    unsigned short* ibData = nullptr;
    if (s_curvedPlaneIB)
        s_pDlIndexBuffers[s_curvedPlaneIB].m_ib->Lock(0, 0, (void**)&ibData, 0);

    s_curvedPlaneIBDataPtr = ibData;
    if (ibData)
        memcpy(ibData, idxData, 0x1080);

    if (s_curvedPlaneIB)
        s_pDlIndexBuffers[s_curvedPlaneIB].m_ib->Unlock();
}

void cloudLayerInitialise()
{
    if (!s_bEnable_2)
        return;

    s_noiseTextures[0] = -1;
    s_noiseTextures[1] = -1;
    dword_82CC4D98 = -1;
    s_densityTex_0 = -1;

    s_curvedPlaneVB = dlVBCreate(1089, 37);
    s_curvedPlaneIB = dlIBCreate(0x1080);

    generateCurvedPlane();
    s_bInitialised_3 = 1;
}

void cloudLayerInitLayer(SCloudLayer_s* pCloudLayer)
{
    if (!s_bEnable_2)
        return;

    s_cloudLayer.m_lightrayStepLength = 1.0f;

    memset(s_cloudLayer.m_noiseOctaveBlendsTemp, 0, 0x20);

    s_cloudLayer.m_windOffset[0] = 0.0f;
    s_cloudLayer.m_windOffset[1] = 0.0f;
    s_cloudLayer.m_bEnable = 1;
    s_cloudLayer.m_sunDistance = 1000000.0f;
    s_cloudLayer.m_bInitialised = 1;

    s_cloudLayer.m_planeCenter.v[0] = 0.0f;
    s_cloudLayer.m_planeCenter.v[1] = s_cloudLayer.m_planeAltitude;
    s_cloudLayer.m_planeCenter.v[2] = 0.0f;
}

// unk_82068FC8 - PBufferOption array for cloud hack preallocated tex buffer
// Original data unknown from dumps, using null options (default behavior)
static const PBufferOption s_hackPreallocatedTexBufferOptions[] = {
    { PBUFFER_CREATE_COLOR_TEXTURE, 1 },
    { (PBufferOptionType)0, 0 }  // terminator
};

void cloudInitialise()
{
    if (s_hackPreallocatedTexBuffer == 0xFFFFFFFF)
        s_hackPreallocatedTexBuffer = pbufferCreate(1024, 1536, 8, (const PBufferOption*)s_hackPreallocatedTexBufferOptions);

    if (s_bEnable_1 && !s_bInitialised_0)
    {
        cloudAreaInitialise();
        cloudAreaInitArea(&s_cloudArea);
        cloudLayerInitialise();
        cloudLayerInitLayer(&s_cloudLayer);
        s_bInitialised_0 = 1;
    }
}

unsigned __int8 s_volumeDataValid[4] = { 0u, 0u, 0u, 0u };
SceneVolumeDataWeights_s s_volumeDataWeights[4];
void sceneVolumeDataValidate(int n)
{
    if (s_volumeDataValid[n])
        return;

    SceneVolumeDataWeights_s* w = &s_volumeDataWeights[n];
    sceneDescriptor_s* sd = &s_sceneDescriptorArray[n].m_sceneDescriptor;
    SceneVolumeData_s* v = &s_volumeData[n];

    s_volumeDataValid[n] = 1;

    // HDR -- lerp from current to target using hdr weight
    float hdr = w->hdr;
    v->hdrLumAlpha = (v->hdrLumAlpha - sd->hdrLumAlpha) * hdr + sd->hdrLumAlpha;
    v->hdrLumWhite = (v->hdrLumWhite - sd->hdrLumWhite) * hdr + sd->hdrLumWhite;
    v->hdrMinAdaptedLum = (v->hdrMinAdaptedLum - sd->hdrMinAdaptedLum) * hdr + sd->hdrMinAdaptedLum;
    v->hdrMaxAdaptedLum = (v->hdrMaxAdaptedLum - sd->hdrMaxAdaptedLum) * hdr + sd->hdrMaxAdaptedLum;
    v->hdrBloomOffset = (v->hdrBloomOffset - sd->hdrBloomOffset) * hdr + sd->hdrBloomOffset;
    v->hdrBloomThreshold = (v->hdrBloomThreshold - sd->hdrBloomThreshold) * hdr + sd->hdrBloomThreshold;
    v->hdrCompression = (v->hdrCompression - sd->hdrCompression) * hdr + sd->hdrCompression;

    // Sun
    float sun = w->sun;
    v->sunAngle = (v->sunAngle - sd->eastAngle) * sun + sd->eastAngle;
    v->spaceSunAngle = (v->spaceSunAngle - sd->spaceEastAngle) * sun + sd->spaceEastAngle;
    v->flareSunAngle = (v->flareSunAngle - sd->flareEastAngle) * sun + sd->flareEastAngle;
    v->sunTimeOfDay = (v->sunTimeOfDay - sd->timeOfDay) * sun + sd->timeOfDay;
    v->spaceSunTimeOfDay = (v->spaceSunTimeOfDay - sd->spaceTimeOfDay) * sun + sd->spaceTimeOfDay;
    v->flareSunTimeOfDay = (v->flareSunTimeOfDay - sd->flareTimeOfDay) * sun + sd->flareTimeOfDay;
    v->sunIntensity = (v->sunIntensity - sd->sunIntensity) * sun + sd->sunIntensity;
    v->spaceSunIntensity = (v->spaceSunIntensity - sd->spaceSunIntensity) * sun + sd->spaceSunIntensity;
    v->finalSunIntensity = (v->finalSunIntensity - sd->finalSunIntensity) * sun + sd->finalSunIntensity;
    v->sunRgb[0] = (v->sunRgb[0] - sd->sunCol.v[0]) * sun + sd->sunCol.v[0];
    v->sunRgb[1] = (v->sunRgb[1] - sd->sunCol.v[1]) * sun + sd->sunCol.v[1];
    v->sunRgb[2] = (v->sunRgb[2] - sd->sunCol.v[2]) * sun + sd->sunCol.v[2];
    v->spaceSunRgb[0] = (v->spaceSunRgb[0] - sd->spaceSunCol.v[0]) * sun + sd->spaceSunCol.v[0];
    v->spaceSunRgb[1] = (v->spaceSunRgb[1] - sd->spaceSunCol.v[1]) * sun + sd->spaceSunCol.v[1];
    v->spaceSunRgb[2] = (v->spaceSunRgb[2] - sd->spaceSunCol.v[2]) * sun + sd->spaceSunCol.v[2];
    v->finalSunRgb[0] = (v->finalSunRgb[0] - sd->finalSunCol.v[0]) * sun + sd->finalSunCol.v[0];
    v->finalSunRgb[1] = (v->finalSunRgb[1] - sd->finalSunCol.v[1]) * sun + sd->finalSunCol.v[1];
    v->finalSunRgb[2] = (v->finalSunRgb[2] - sd->finalSunCol.v[2]) * sun + sd->finalSunCol.v[2];
    v->sunDraw = (sun <= 0.0f) ? sd->sunLightEnabled : v->sunDraw;

    // Flare
    float flare = w->flare;
    v->flareRange = (v->flareRange - sd->flareRange) * flare + sd->flareRange;
    v->flareScale = (v->flareScale - sd->flareScale) * flare + sd->flareScale;
    v->flareMaxAlpha = (v->flareMaxAlpha - sd->flareMaxAlpha) * flare + sd->flareMaxAlpha;
    v->flareJitter = (v->flareJitter - sd->flareJitter) * flare + sd->flareJitter;
    v->flareNoise = (v->flareNoise - sd->flareNoise) * flare + sd->flareNoise;
    v->flareDraw = (flare <= 0.0f) ? sd->flareOn : v->flareDraw;

    // Sky
    float sky = w->sky;
    v->skyAmbientRgb[0] = (v->skyAmbientRgb[0] - sd->skyAmbCol.v[0]) * sky + sd->skyAmbCol.v[0];
    v->skyAmbientRgb[1] = (v->skyAmbientRgb[1] - sd->skyAmbCol.v[1]) * sky + sd->skyAmbCol.v[1];
    v->skyAmbientRgb[2] = (v->skyAmbientRgb[2] - sd->skyAmbCol.v[2]) * sky + sd->skyAmbCol.v[2];
    v->skyAmbientBoost = (v->skyAmbientBoost - sd->skyAmbColScale) * sky + sd->skyAmbColScale;

    // Fog
    float fog = w->fog;
    v->fogCol[0] = (v->fogCol[0] - sd->filters.fog[0].colour.v[0]) * fog + sd->filters.fog[0].colour.v[0];
    v->fogCol[1] = (v->fogCol[1] - sd->filters.fog[0].colour.v[1]) * fog + sd->filters.fog[0].colour.v[1];
    v->fogCol[2] = (v->fogCol[2] - sd->filters.fog[0].colour.v[2]) * fog + sd->filters.fog[0].colour.v[2];
    v->fogIntensity = (v->fogIntensity - sd->filters.fog[0].colour.v[3]) * fog + sd->filters.fog[0].colour.v[3];
    v->fogNear = (v->fogNear - sd->filters.fog[0].nearPlane) * fog + sd->filters.fog[0].nearPlane;
    v->fogFar = (v->fogFar - sd->filters.fog[0].farPlane) * fog + sd->filters.fog[0].farPlane;
    v->fogSky = (fog <= 0.0f) ? sd->filters.fog[0].sky : v->fogSky;

    // DOF / mono / rain
    float dof = w->dof;
    float mono = w->mono;
    float rain = w->rain;
    v->dofIntensity = (v->dofIntensity - sd->filters.dofAmount) * dof + sd->filters.dofAmount;
    v->dofNear = (v->dofNear - sd->filters.dofNear) * dof + sd->filters.dofNear;
    v->dofFar = (v->dofFar - sd->filters.dofFar) * dof + sd->filters.dofFar;
    v->monoIntensity = (v->monoIntensity - sd->filters.monochrome) * mono + sd->filters.monochrome;
    v->rainIntensity = (v->rainIntensity - sd->filters.rainDensity) * rain + sd->filters.rainDensity;
    v->lightning = (rain <= 0.0f) ? sd->lightning : v->lightning;

    // Noise / colour grading
    float noise = w->noise;
    float mulRgb = w->mulRgb;
    float addRgb = w->addRgb;
    float addScr = w->addScr;
    v->noiseIntensity = (v->noiseIntensity - sd->filters.noise) * noise + sd->filters.noise;
    v->mulRgbCol[0] = (v->mulRgbCol[0] - sd->filters.mulRgb.v[0]) * mulRgb + sd->filters.mulRgb.v[0];
    v->mulRgbCol[1] = (v->mulRgbCol[1] - sd->filters.mulRgb.v[1]) * mulRgb + sd->filters.mulRgb.v[1];
    v->mulRgbCol[2] = (v->mulRgbCol[2] - sd->filters.mulRgb.v[2]) * mulRgb + sd->filters.mulRgb.v[2];
    v->addRgbCol[0] = (v->addRgbCol[0] - sd->filters.addRgb.v[0]) * addRgb + sd->filters.addRgb.v[0];
    v->addRgbCol[1] = (v->addRgbCol[1] - sd->filters.addRgb.v[1]) * addRgb + sd->filters.addRgb.v[1];
    v->addRgbCol[2] = (v->addRgbCol[2] - sd->filters.addRgb.v[2]) * addRgb + sd->filters.addRgb.v[2];
    v->addScrCol[0] = (v->addScrCol[0] - sd->filters.addScr.v[0]) * addScr + sd->filters.addScr.v[0];
    v->addScrCol[1] = (v->addScrCol[1] - sd->filters.addScr.v[1]) * addScr + sd->filters.addScr.v[1];
    v->addScrCol[2] = (v->addScrCol[2] - sd->filters.addScr.v[2]) * addScr + sd->filters.addScr.v[2];

    // CML (colour matrix lighting)
    float cml = w->cml;
    float cmlScale = (v->cmlScale - 1.0f) * cml + 1.0f;
    v->cmlSunClamp = (v->cmlSunClamp - sd->CMLSunClamp) * cml + sd->CMLSunClamp;
    v->cmlScale = cmlScale;
    v->cmlSunScale = (v->cmlSunScale - sd->CMLintensscale) * cml + sd->CMLintensscale;
    for (int i = 0; i < 6; i++)
        v->cmlScaleFace[i] = (cmlScale * v->cmlScaleFace[i] - 1.0f) * cml + 1.0f;
    v->cmlDetach = (cml <= 0.0f) ? 0 : v->cmlDetach;
}

float s_impostorsInvalidateAngle = 0.017453292f; // PI / 180
void sceneSetWhichEastAngle(float eastAngle, vec3_u* eastVec)
{
    float radians = eastAngle * s_impostorsInvalidateAngle; // deg to rad scale

    float c = cosf(radians);
    float s = sinf(radians);

    eastVec->v[0] = c;
    eastVec->v[1] = 0.0f;
    eastVec->v[2] = s;

    vecnormaliseGold(eastVec, eastVec);
}

void sceneSetEastAngle(int num, float east)
{
    sceneDescriptor_s* sd = &s_sceneDescriptorArray[num].m_sceneDescriptor;

    sd->eastAngle = east;

    if (rdebugflags.linkFlarePosWithSun)
        sd->flareEastAngle = east;

    sceneSetWhichEastAngle(east, &sd->east);

    if (rdebugflags.linkFlarePosWithSun)
    {
        sd->flareEast.v[0] = sd->east.v[0];
        sd->flareEast.v[1] = sd->east.v[1];
        sd->flareEast.v[2] = sd->east.v[2];
    }
    else
    {
        sceneSetWhichEastAngle(sd->flareEastAngle, &sd->flareEast);
    }
}

void sceneProcessSunDirPos(
    vec3_u* dir, vec3_u* pos,
    float timeOfDay, float timeSunRise, float timeSunSet,
    vec3_u* east)
{
    float t;

    if (timeOfDay >= timeSunRise && timeOfDay <= timeSunSet)
    {
        // Daytime -- normalize within the day window
        float dayLen = timeSunSet - timeSunRise;
        if (fabsf(dayLen) > 0.0f)
            t = (timeOfDay - timeSunRise) / dayLen;
        else
            t = timeOfDay - timeSunRise;
    }
    else
    {
        // Nighttime -- normalize within the night window
        float nightLen = 24.0f - (timeSunSet - timeSunRise);
        if (timeOfDay >= timeSunSet)
            t = (timeOfDay - timeSunSet) / nightLen + 1.0f;
        else
            t = (nightLen - (timeSunRise - timeOfDay)) / nightLen + 1.0f;
    }

    // Convert to angle (0..PI = daytime arc)
    float angle = t * 3.1415927f;

    float c = cosf(angle);
    float s = sinf(angle);

    // Build sun direction using east vector
    dir->v[0] = east->v[0] * c;
    dir->v[1] = -s;
    dir->v[2] = east->v[2] * c;
    vecnormaliseGold(dir, dir);

    // Sun position = direction * -AU (astronomical unit in km)
    const float AU = 149597870.0f;
    const float planetRadius = 64000.0f;   // k_planetRadius

    pos->v[0] = dir->v[0] * -AU;
    pos->v[1] = dir->v[1] * -AU;
    pos->v[2] = dir->v[2] * -AU;

    // Offset Y by planet radius
    pos->v[1] = pos->v[1] - planetRadius;
}

void lightSetPosition(unsigned int lightH, const vec3_u* pos)
{
    if (lightH == (unsigned int)-1)
        return;

    unsigned int handle = worldlights[lightH].handle;
    if (!handle)
        return;

    // Validate handle is still live
    if (worldlights[handle >> 20].handle != handle)
    {
        worldlights[lightH].handle = 0;
        return;
    }

    LightBase* light = &worldlights[lightH];
    light->room.room = -1;
    light->room.bgih = -1;
    light->pos = *pos;
}

void lightSetDirection(unsigned int light, const vec3_u* direction)
{
    if (light == (unsigned int)-1)
        return;

    unsigned int handle = worldlights[light].handle;
    if (!handle)
        return;

    // Validate handle is still live
    if (worldlights[handle >> 20].handle != handle)
    {
        worldlights[light].handle = 0;
        return;
    }

    worldlights[light].spot.dir = *direction;
}

void lightUpdateCachedColour(unsigned int light)
{
    if (light == (unsigned int)-1)
        return;

    unsigned int handle = worldlights[light].handle;
    if (!handle)
        return;

    if (worldlights[handle >> 20].handle != handle)
    {
        worldlights[light].handle = 0;
        return;
    }

    float scale = worldlights[light].invIntensity * worldlights[light].intensity;

    worldlights[light].cachedColour[0] = worldlights[light].colour[0] * scale;
    worldlights[light].cachedColour[1] = worldlights[light].colour[1] * scale;
    worldlights[light].cachedColour[2] = worldlights[light].colour[2] * scale;
}

void lightSetColour(unsigned int light, float red, float green, float blue)
{
    if (light == (unsigned int)-1)
        return;

    unsigned int handle = worldlights[light].handle;
    if (!handle)
        return;

    if (worldlights[handle >> 20].handle != handle)
    {
        worldlights[light].handle = 0;
        return;
    }

    worldlights[light].colour[0] = red;
    worldlights[light].colour[1] = green;
    worldlights[light].colour[2] = blue;
    lightUpdateCachedColour(light);
}

void sceneUpdateSun(int num)
{
    CPerViewSceneDescriptorSettings* v1 = &s_sceneDescriptorArray[num];
    sceneDescriptor_s* sd = &s_sceneDescriptorArray[num].m_sceneDescriptor;

    float timeSunRise = sd->timeSunRise;
    float timeSunSet = sd->timeSunSet;

    sceneVolumeDataValidate(num);
    float flareSunTimeOfDay = s_volumeData[num].flareSunTimeOfDay;

    // Sun direction and position
    sceneProcessSunDirPos(
        &sd->sunDir,
        &sd->sunPos,
        sd->timeOfDay,
        timeSunRise,
        timeSunSet,
        &sd->east);

    // Space sun direction and position
    sceneProcessSunDirPos(
        &sd->spaceSunDir,
        &sd->spaceSunPos,
        sd->spaceTimeOfDay,
        timeSunRise,
        timeSunSet,
        &sd->spaceEast);

    lightSetDirection(v1->m_sunLight, &sd->sunDir);
    lightSetPosition(v1->m_sunLight, &sd->sunPos);

    // Flare sun direction and position
    sceneProcessSunDirPos(
        &sd->flareSunDir,
        &sd->flareSunPos,
        flareSunTimeOfDay,
        timeSunRise,
        timeSunSet,
        &sd->flareEast);

    // Set sun light colour
    if (v1->m_sunLight != (unsigned int)-1)
    {
        lightSetColour(
            v1->m_sunLight,
            sd->sunCol.v[0] * sd->sunIntensity,
            sd->sunCol.v[1] * sd->sunIntensity,
            sd->sunCol.v[2] * sd->sunIntensity);
    }
}

void sceneDescriptorUpdateSun(int num)
{
    sceneDescriptor_s* sd = &s_sceneDescriptorArray[num].m_sceneDescriptor;

    sceneVolumeDataValidate(num);

    // Convert sunTimeOfDay to seconds (multiply by 3600)
    float realTime = s_volumeData[num].sunTimeOfDay * 3600.0f;
    sd->realTimeOfDay = realTime;

    // Wrap at 86400 seconds (24 hours)
    if (realTime > 86400.0f)
        sd->realTimeOfDay = realTime - 86400.0f;

    // Sync flare time of day with sun if debug flag set
    if (rdebugflags.linkFlarePosWithSun)
    {
        sceneVolumeDataValidate(num);
        sd->flareTimeOfDay = s_volumeData[num].sunTimeOfDay;
    }

    sceneSetEastAngle(num, sd->eastAngle);

    // Store spaceEastAngle (no-op self assignment in original, just passes through)
    sceneSetWhichEastAngle(sd->spaceEastAngle, &sd->spaceEast);

    sceneUpdateSun(num);
}

unsigned __int8 s_CMLLightEnable = 0u;
const char* s_noiseTexturesName[2] = { NULL, NULL };
const char* s_noiseOctaveTexName[2] = { NULL, NULL };
int s_framebufferTexWidth = 0;
int s_framebufferTexHeight = 0;
int s_framebufferWidth = 0;
int s_impostorsTexSize = 0;
float s_transparencyZNear = 0.0;
float s_transparencyZFar = 0.0;
vec4_u s_colour = { { 0.0, 0.0, 0.0, 0.0 } };
unsigned __int8 s_enabled_0 = 0u;
unsigned __int8 s_initialised_0 = 0u;
float s_impostorsLevelScaleThreshold[4] = { 0.0, 0.0, 0.0, 0.0 };
float s_skyLightningFlash[8] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
float s_skyLightningWait[8] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
unsigned __int64 newrndseedval = 558189139uLL;

void skyLightningUpdate(unsigned int viewIndex)
{
    if (viewIndex > 7)
        return;

    sceneVolumeDataValidate(viewIndex);

    int idx = (int)viewIndex;

    if (s_skyLightningFlash[idx] > 0.0f)
    {
        // Flash is active -- count it down
        s_skyLightningFlash[idx] -= timerLastGameFrameDuration * c_PhantomMovementExpansionRatio;
        return;
    }

    if (!s_volumeData[idx].lightning)
        return;

    // Count down the wait timer
    s_skyLightningWait[idx] -= timerLastGameFrameDuration;

    if (s_skyLightningWait[idx] >= 0.0f)
        return;

    // Timer expired -- trigger new lightning using LCG random
    newrndseedval = newrndseedval * 214013LL + 2531011LL;
    long long seed1 = newrndseedval;
    newrndseedval = newrndseedval * 214013LL + 2531011LL;
    long long seed2 = newrndseedval;

    float r1 = (float)((seed1 >> 33) & 0x7FFF) * 0.000030518509f;
    float r2 = (float)((seed2 >> 33) & 0x7FFF) * 0.000030518509f;

    // New wait time: random in [1.5, 5.0]
    s_skyLightningWait[idx] = r1 * 3.5f + 1.5f;

    // New flash duration: random in [1.5, 3.0]
    s_skyLightningFlash[idx] = r2 * 1.5f + 1.5f;
}

void lightSetEnable(unsigned int light, int on)
{
    if (light == (unsigned int)-1)
        return;

    unsigned int handle = worldlights[light].handle;
    if (!handle)
        return;

    if (worldlights[handle >> 20].handle != handle)
    {
        worldlights[light].handle = 0;
        return;
    }

    if (on)
        worldlights[light].flags |= 1;
    else
        worldlights[light].flags &= ~1;
}

void hackReleaseTexBuffer()
{
    if (s_hackPreallocatedTexBuffer == -1)
        return;

    if (sFrameBuffer != (unsigned int)s_hackPreallocatedTexBuffer &&
        s_hackPreallocatedTexBuffer < 0x40)
    {
        if (pbuffers[s_hackPreallocatedTexBuffer].valid)
            pbufferReleaseHW(s_hackPreallocatedTexBuffer);
    }

    s_hackPreallocatedTexBuffer = -1;
}

void sceneDescriptorPrepareForRender(int i, viewdef_s* view)
{
    CPerViewSceneDescriptorSettings* v4 = &s_sceneDescriptorArray[i];
    sceneDescriptor_s* sd = &s_sceneDescriptorArray[i].m_sceneDescriptor;

    s_renderingUsingSceneDescriptorIndex = i;
    sceneDescriptorUpdateSun(i);
    skyLightningUpdate(i);
    sceneVolumeDataValidate(i);

    lightSetEnable(s_sceneDescriptorArray[i].m_sunLight, s_volumeData[i].sunDraw);

    if (v4->m_set && !rdebugflags.editSceneDescriptors)
    {
        s_bEnable_1 = sd->cloudEnable;
        if (!s_bEnable_1)
            hackReleaseTexBuffer();

        s_bEnable_2 = sd->cloudLayerEnable;
        s_bEnable = sd->cloudAreaEnable;

        // Cloud layer noise
        for (int j = 0; j < 8; j++)
        {
            s_cloudLayer.m_noiseOctaveWeights[j] = sd->cloudLayerNoiseOctaveWeights[j];
            s_cloudLayer.m_noiseOctaveEvolFrequencies[j] = sd->cloudLayerNoiseOctaveEvolFreqs[j];
        }

        s_cloudLayer.m_cloudHalfHeight = sd->cloudLayerHalfHeight;
        s_cloudLayer.m_cloudCover = sd->cloudLayerCover;
        s_cloudLayer.m_cloudSharpness = sd->cloudLayerSharpness;
        s_cloudLayer.m_lightScattering = sd->cloudLayerLightScattering;
        s_cloudLayer.m_lightrayStepLength = sd->cloudLayerLightrayStepLength;
        s_cloudLayer.m_cloudMaxLighting = sd->cloudLayerMaxLighting;
        s_cloudLayer.m_cloudMinLighting = sd->cloudLayerMinLighting;
        s_cloudLayer.m_cloudNoiseTexTilingScale = sd->cloudLayerNoiseTexTilingScale;
        s_cloudLayer.m_planeSizeScale = sd->cloudLayerPlaneSizeScale;
        s_cloudLayer.m_planeAltitude = sd->cloudLayerPlaneAltitude;
        s_curvedPlaneRadius = sd->cloudLayerCurvedPlaneRadius;
        s_cloudLayer.m_windSpeed = sd->cloudLayerWindSpeed;
        s_cloudLayer.m_windAngleFromXAxis = sd->cloudLayerWindAngleFromXAxis;

        s_cloudLayer.m_cloudLightColor.v[0] = sd->cloudLayerLightColor.v[0];
        s_cloudLayer.m_cloudLightColor.v[1] = sd->cloudLayerLightColor.v[1];
        s_cloudLayer.m_cloudLightColor.v[2] = sd->cloudLayerLightColor.v[2];
        s_cloudLayer.m_cloudDarkColor.v[0] = sd->cloudLayerDarkColor.v[0];
        s_cloudLayer.m_cloudDarkColor.v[1] = sd->cloudLayerDarkColor.v[1];
        s_cloudLayer.m_cloudDarkColor.v[2] = sd->cloudLayerDarkColor.v[2];

        s_noiseTexturesName[0] = sd->cloudLayerNoiseTex0Name;
        s_noiseTexturesName[1] = sd->cloudLayerNoiseTex1Name;
        s_noiseOctaveTexName[0] = sd->cloudAreaNoiseTexName;

        s_framebufferTexWidth = sd->cloudVolumeFramebufferTexWidth;
        s_framebufferTexHeight = sd->cloudVolumeFramebufferTexHeight;
        s_framebufferWidth = sd->cloudVolumeFramebufferWidth;
        s_sliceCount = sd->cloudVolumeSliceCount;
        s_sliceTesselation = sd->cloudVolumeSliceTesselation;
        s_impostorsTexSize = sd->cloudVolumeImpostorsTexSize;
        s_transparencyZNear = sd->cloudVolumeTransparencyZNear;
        s_transparencyZFar = sd->cloudVolumeTransparencyZFar;

        for (int j = 0; j < 4; j++)
            s_impostorsLevelScaleThreshold[j] = sd->cloudVolumeImpostorsLevelScaleThreshold[j];

        s_cloudArea.m_cloudCount = sd->cloudAreaCloudCount;
        s_cloudArea.m_bNoIntersectingClouds = sd->cloudAreaNoIntersectingClouds;
        s_cloudArea.m_cloudSharpness = sd->cloudAreaSharpness;
        s_cloudArea.m_lightScattering = sd->cloudAreaLightScattering;
        s_cloudArea.m_cloudMaxLighting = sd->cloudAreaMaxLighting;
        s_cloudArea.m_cloudMinLighting = sd->cloudAreaMinLighting;
        s_cloudArea.m_noiseTexScaleTiling = sd->cloudAreaNoiseTexScaleTiling;
        s_cloudArea.m_noiseInfluence = sd->cloudAreaNoiseInfluence;

        s_cloudArea.m_cloudAreaAABB.bb[0].v[0] = sd->cloudAreaMin.v[0];
        s_cloudArea.m_cloudAreaAABB.bb[0].v[1] = sd->cloudAreaMin.v[1];
        s_cloudArea.m_cloudAreaAABB.bb[0].v[2] = sd->cloudAreaMin.v[2];
        s_cloudArea.m_cloudAreaAABB.bb[1].v[0] = sd->cloudAreaMax.v[0];
        s_cloudArea.m_cloudAreaAABB.bb[1].v[1] = sd->cloudAreaMax.v[1];
        s_cloudArea.m_cloudAreaAABB.bb[1].v[2] = sd->cloudAreaMax.v[2];

        s_cloudArea.m_cloudSizeMin.v[0] = sd->cloudAreaCloudSizeMin.v[0];
        s_cloudArea.m_cloudSizeMin.v[1] = sd->cloudAreaCloudSizeMin.v[1];
        s_cloudArea.m_cloudSizeMin.v[2] = sd->cloudAreaCloudSizeMin.v[2];
        s_cloudArea.m_cloudSizeMax.v[0] = sd->cloudAreaCloudSizeMax.v[0];
        s_cloudArea.m_cloudSizeMax.v[1] = sd->cloudAreaCloudSizeMax.v[1];
        s_cloudArea.m_cloudSizeMax.v[2] = sd->cloudAreaCloudSizeMax.v[2];

        s_cloudArea.m_lightColor.v[0] = sd->cloudAreaLightColor.v[0];
        s_cloudArea.m_lightColor.v[1] = sd->cloudAreaLightColor.v[1];
        s_cloudArea.m_lightColor.v[2] = sd->cloudAreaLightColor.v[2];
        s_cloudArea.m_darkColor.v[0] = sd->cloudAreaDarkColor.v[0];
        s_cloudArea.m_darkColor.v[1] = sd->cloudAreaDarkColor.v[1];
        s_cloudArea.m_darkColor.v[2] = sd->cloudAreaDarkColor.v[2];

        for (int j = 0; j < 8; j++)
            s_cloudArea.m_noiseOctaveWeights[j] = sd->cloudAreaOctaveWeight[j];
    }

    // HDR enable
    unsigned __int8 hdrEnabled = sd->hdrEnabled;
    if (!sForceLDR)
    {
        if (hdrEnabled && !sHdrEnabledNext)
        {
            s_hdrFirstFrame[0] = 1;
            s_hdrFirstFrame[1] = 1;
            s_hdrFirstFrame[2] = 1;
            s_hdrFirstFrame[3] = 1;
        }
        sHdrEnabledNext = hdrEnabled;
    }

    // HDR volume data
    sceneVolumeDataValidate(i);
    s_hdrMaxAdaptedLum = s_volumeData[i].hdrMaxAdaptedLum;
    s_hdrAdaptTimeScale = sd->hdrAdaptTimeScale;
    s_hdrSampleRadius = 1.0f / sd->hdrSampleRadius;
    s_hdrMinAdaptedLum = s_volumeData[i].hdrMinAdaptedLum;

    sceneVolumeDataValidate(i);
    float hdrLumWhite = s_volumeData[i].hdrLumWhite;

    sceneVolumeDataValidate(i);
    float hdrLumAlpha2 = s_volumeData[i].hdrLumAlpha;

    sceneVolumeDataValidate(i);
    s_hdrPowerLumCalc = sd->hdrPowerLuminanceCalc;
    s_hdrLumWhiteSq = hdrLumAlpha2 * hdrLumWhite;
    s_hdrLumAlpha = s_volumeData[i].hdrLumAlpha;

    sceneVolumeDataValidate(i);
    s_hdrBloomThreshold = s_volumeData[i].hdrBloomThreshold;

    sceneVolumeDataValidate(i);
    s_hdrBloomOffset = s_volumeData[i].hdrBloomOffset;

    sceneVolumeDataValidate(i);
    s_hdrCompression = s_volumeData[i].hdrCompression;

    // Sun change detection -- cached in s_CMLLightEnable block
    float* cmlCache = (float*)((char*)&s_CMLLightEnable + 0xBB2);
    float cachedTimeOfDay = cmlCache[0];
    float cachedEastAngle = cmlCache[1];
    float cachedSpaceTime = cmlCache[2];
    float cachedSpaceAngle = cmlCache[3];

    if (cachedTimeOfDay == sd->timeOfDay &&
        cachedEastAngle == sd->eastAngle &&
        cachedSpaceTime == sd->spaceTimeOfDay &&
        cachedSpaceAngle == sd->spaceEastAngle)
    {
        sd->forceUpdate = 0;
    }

    cmlCache[1] = sd->eastAngle;
    cmlCache[2] = sd->spaceTimeOfDay;
    cmlCache[0] = sd->timeOfDay;
    cmlCache[3] = sd->spaceEastAngle;

    // Mist colour
    if (sd->mistColour.v != nullptr)
    {
        s_colour.v[0] = sd->mistColour.v[0];
        s_colour.v[1] = sd->mistColour.v[1];
        s_colour.v[2] = sd->mistColour.v[2];
        s_colour.v[3] = sd->mistColour.v[3];
    }

    // Mist enable
    if (!sd->mistEnabled)
        s_enabled_0 = 0;
    else if (s_initialised_0)
        s_enabled_0 = 1;

    // CML light enable
    s_CMLLightEnable = (sd->lightHackEnable != 0) ? 1 : 0;
}