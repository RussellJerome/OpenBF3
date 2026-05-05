#pragma once
#include "mathf.h"
#include <windows.h>
#include <iostream>
#include "engine/string/engstring.h"

//A massive bunch of unorganized types. TODO: Organize ig

struct HOM;
struct ShaderTreeDefinition;
union MenuUnion_s;

enum ETaskType : __int32
{
    SKINNING_TASK = 0x0,
    TEST_TASK = 0x1,
    PARTICLE_TASK = 0x2,
    M0V_TASK = 0x3,
    PAK_TASK = 0x4,
    LINETEST_TASK = 0x5,
    FRD_LINETEST_TASK = 0x6,
    NAVROUTE_TASK = 0x7,
    CLOTH_TASK = 0x8,
    PMESH_TASK = 0x9,
    ABM_TASK = 0xA,
    HOM_TASK = 0xB,
    DATA_TRANSFER_TASK = 0xC,
    NUM_TASK_TYPES = 0xD,
};

struct __declspec(align(2)) $4A1482FBCCD522B5B4CFDEF3CFC6ED79
{
    unsigned int id;
    ETaskType type;
    unsigned __int8 status;
    unsigned __int8 scheduleType;
};

union $286E6D0C4CB27190F2F64704DDAC088B
{
    $4A1482FBCCD522B5B4CFDEF3CFC6ED79 __s0;
    __vector4 dummyAlign;
};

struct heightMapTileInfo_s
{
    int xoff;
    int yoff;
    int width;
    int height;
    int samplesX;
    int samplesY;
    int resolution;
    __int16* data;
};

struct heightMapInfo_s
{
    unsigned int numTiles;
    unsigned int extraTiles;
    heightMapTileInfo_s* infos[1];
};

struct $2DA0A92846D909C463505992F2CB011D
{
    vec3_u min;
    vec3_u max;
};

union aabbdef_u
{
    vec3_u bb[2];
    $2DA0A92846D909C463505992F2CB011D __s1;
};

struct lightvolCML_s
{
    unsigned __int16 c[6];
};

struct lightvolhiernode_s
{
    __int16 idx[8];
};

struct lightvol_s
{
    unsigned int numhiernodes;
    unsigned int numsamppnts;
    aabbdef_u bb;
    lightvolCML_s* samppnts;
    lightvolhiernode_s* hier;
};

struct $960343A40258BC691E01FF4808B7193B
{
    float x;
    float z;
};

struct $5B4BB5DE427D79AA0465BBBB03BF820D
{
    $960343A40258BC691E01FF4808B7193B min;
    $960343A40258BC691E01FF4808B7193B max;
};

struct TTTRHeader
{
    unsigned int magic;
    unsigned int version;
    unsigned int w;
    unsigned int h;
    float xScale;
    float zScale;
    unsigned int layers;
    unsigned int detail;
    unsigned int colour[3];
    unsigned int normal[2];
    unsigned int blendmap;
    unsigned int prebake;
    unsigned int fourKTex;
    unsigned int eightKTex;
    unsigned int sixteenKTex;
    unsigned int sixtyfourKTex;
    unsigned int lightmap;
    unsigned int mega;
    float uScale;
    float vScale;
    unsigned int foliage;
    heightMapInfo_s* heightData;
    lightvol_s* cubemapLighting;
    float hScale;
    float hOffset;
    unsigned int numPhysicsMaterials;
    $5B4BB5DE427D79AA0465BBBB03BF820D lightmapBox;
};

struct heightMap_s
{
    unsigned __int8* data;
    TTTRHeader* ttrHeader;
    int width;
    int height;
    float scale;
    float offset;
};

struct $38C3DEC81229B66F67FB6D350D75FF5A
{
    float x;
    float y;
};

union vec2_u
{
    float v[2];
    $38C3DEC81229B66F67FB6D350D75FF5A __s1;
};

struct __declspec(align(4)) pmeshLevel_s
{
    float size;
    vec2_u loc;
    vec2_u nloc;
    vec2_u centre;
    int numIndices;
    int maxIndices;
    unsigned int idxBuffers[4];
    unsigned __int16* idxBuffersPtr[4];
    int shadowIdxBuffer;
    int currentIdxBuffer;
    unsigned int heightNormBuffer;
    float* vertnormBuffer;
    float* lheightMap;
    int* batchesPolyCount;
    int shadowPolyCount;
    int invalid;
    unsigned __int8 levelTypeFlag;
};

union $3B4C8F8E725C58DDCE709F30443795F7
{
    vec4_u plane;
    __vector4 hwplane;
};

struct __declspec(align(8)) frustumdatadef_s
{
    $3B4C8F8E725C58DDCE709F30443795F7 ___u0;
    int nx;
    int ny;
    int nz;
};

struct frustumdef_s
{
    frustumdatadef_s planeData[6];
};

struct terrainArea_s
{
    int minBlockIndexX;
    int minBlockIndexY;
    int maxBlockIndexX;
    int maxBlockIndexY;
};

struct texturedArea_s
{
    terrainArea_s area;
    unsigned int tex;
};

struct __declspec(align(16)) pmesh_s
{
    int created;
    heightMap_s hdata;
    pmeshLevel_s* pViewLevels[4][2];
    unsigned int bViewLevelsUpdateFinished[4][2];
    int numLevels;
    vec3_u viewPos[4];
    frustumdef_s pmeshSpaceviewFrustum[4];
    float minLevel[4];
    int drawThisFrame[4];
    int levelBatchCount;
    int terrainBlockCountAlongX;
    int terrainBlockCountAlongY;
    float terrainOffsetX;
    float terrainOffsetY;
    unsigned int specularIntensity;
    unsigned int colour[3];
    unsigned int normal[2];
    unsigned int blendMap;
    unsigned int lightMap;
    unsigned int outerLightMap;
    unsigned int SixtyFourKTex;
    int currentArea;
    texturedArea_s staticAreas[3];
    char* texUsePerBlock;
    int editable;
    int numRenderLayers;
    int(__cdecl* renderLayerCallback[8])(int, int*);
    int numEditLevels;
    float editLevelSize[4];
    char* physicsMaterialsNames;
    $5B4BB5DE427D79AA0465BBBB03BF820D detailLightmap;
    void* altData;
    unsigned __int8 skipRender;
    unsigned __int8 shouldDrawOuterLayers;
};

struct BgRoomGroup_s
{
    unsigned int bgih;
    int roomGroup;
};

struct SPmeshTaskArgs_s
{
    unsigned int* pTaskID;
    const pmesh_s* mesh;
    int pmeshViewIndex;
    int pmeshRingBufferIndex;
    const BgRoomGroup_s* bgRoomGroup;
    __declspec(align(16)) frustumdef_s shadowFrust;
    unsigned int bUseShadow;
};

struct $0D0CB43DF22755AD856C77DD3F304010
{
    unsigned __int8 r;
    unsigned __int8 g;
    unsigned __int8 b;
    unsigned __int8 a;
};

struct $28A82F0D82CD7F7D039332613D1237DA
{
    unsigned __int8 x;
    unsigned __int8 y;
    unsigned __int8 z;
    unsigned __int8 w;
};

union vec4u8_u
{
    $28A82F0D82CD7F7D039332613D1237DA __s0;
    unsigned __int8 v[4];
    $0D0CB43DF22755AD856C77DD3F304010 __s2;
};

struct $B6CDCBCA624636590912201CFB7D394C
{
    vec3_u v;
    float w;
};

union quaternion_u
{
    float q[4];
    $B6CDCBCA624636590912201CFB7D394C __s1;
    $393C16A032292777F0C3725FFB2C0008 __s2;
};

struct SSkinningInputVertex_s
{
    vec3_u position;
    vec4u8_u matrixIdx;
    vec4_u matrixWeight;
    quaternion_u packSTN;
};

struct SSkinningOutputVertex_s
{
    vec3_u position;
    float pad;
    quaternion_u stn;
};

struct SSkinningTaskArgs_s
{
    unsigned int numVertices;
    unsigned __int16 numMatrices;
    unsigned __int8 compressed;
    unsigned __int8 pad;
    SSkinningInputVertex_s* inVertices;
    mtx_u* inMatrices;
    SSkinningOutputVertex_s* outVertices;
    quaternion_u* inQuatData;
    void* bufferHandle;
    unsigned int fence;
};

struct SParticleTaskArgs_s
{
    void* queuedSystems;
    unsigned int numQueuedSystems;
    unsigned int startIndex;
    unsigned int stepSize;
    float dt;
};

struct STestTaskArgs_s
{
    unsigned int taskNumber;
};

struct tm0vQue
{
    int yCount;
    void* pTaskID;
    unsigned __int8* pCodeBook;
    char* pCodeVect;
    char* pYUV;
    unsigned __int8* pRGB;
    unsigned __int16* pDecompBook;
    unsigned __int16 Width;
    unsigned __int16 FrameIndex;
    unsigned __int8 CodeBookBit;
    unsigned __int8 CodeBookBits;
    unsigned __int8 CompressCodeBook;
    unsigned __int8 YUVLayout;
    unsigned __int8 EndianSwap;
    unsigned __int8 pad[3];
};

struct tpakItem
{
    void* ppak;
};

struct tpakQue
{
    tpakItem item;
    volatile int* preturnCode;
    unsigned __int8 pad[24];
};

struct SLinetestArgs_s
{
    void* tltBatch;
    void* batchMgr;
    unsigned int linetestTaskNum;
    unsigned int linetestsOnTask;
    void* dataPtr;
};

struct SFrdLinetestArgs_s
{
    void* data;
};

struct SNavRouteFindArgs_s
{
    unsigned __int16 startVol;
    unsigned __int16 endVol;
};

struct SClothThreadArgs_s
{
    void* cloths[32];
    int sizeOfCloths[32];
    unsigned int numCloths;
    float dt;
};

enum EABMTaskState : __int32
{
    k_abmtask_building = 0x0,
    k_abmtask_processing = 0x1,
    k_abmtask_complete = 0x2,
};

enum EABMTaskResultType : __int32
{
    k_abmtaskresulttype_mtx = 0x0,
    k_abmtaskresulttype_animData = 0x1,
};

struct animData_s
{
    quaternion_u q;
    vec3_u t;
    float pad;
};

struct SABMBlendResults
{
    unsigned int m_abmFlags;
    unsigned int m_btrFlags;
    float m_blockFacialScalar;
    float m_blockUbiksScalar;
    float m_blockOverlayScalar;
    vec3_u m_posDelta;
    float m_bodyOrientYRot;
    vec3_u m_bodyTrans;
};


struct SABMProgramClientState
{
    unsigned __int8 m_selMasks[3];
    int m_selMasksStackIndex;
    SABMBlendResults m_blendResultsStack[9];
    int m_blendResultsStackIndex;
    unsigned int m_animModel;
};

union $6578E2CE6FB62B01F0501B9556CC3DE2
{
    __vector4 m_alignTo16Bytes;
    int m_cmds[400];
};

struct __declspec(align(16)) SABMProgram
{
    $6578E2CE6FB62B01F0501B9556CC3DE2 ___u0;
    int m_cmdsUsed;
    SABMProgramClientState* m_clientState;
};

struct obSkeletonDef_s
{
    char* name;
    unsigned int nameHash;
    __int16 parent;
    __int16 __unused_was_type;
    float inv_qRot[4];
    float inv_trans[3];
    float qRot[4];
    float trans[3];
};

struct SAnimCoreSkeleton
{
    int numMtxs;
    const obSkeletonDef_s* skelData;
    float scale;
};

union UABMSubTaskResults_s
{
    __vector4 mustBeAlignedTo16;
    mtx_u resultMtx[1];
    animData_s resultAD[1];
};

struct SABMSubTask_s
{
    unsigned int abmTaskId;
    EABMTaskResultType wantResultsAs;
    UABMSubTaskResults_s* results;
    int numResultEntries;
    SABMProgram* prog;
    SAnimCoreSkeleton skel;
};

struct $57053AA6EC86A069F778CFA79F988BB0
{
    unsigned int taskManId;
    volatile EABMTaskState state;
    int numSubTasks;
    SABMSubTask_s subTasks[2];
};

union __declspec(align(8)) SABMTask
{
    __vector4 padTo16Bytes;
    $57053AA6EC86A069F778CFA79F988BB0 __s1;
};

struct SABMTaskArgs_s
{
    SABMTask* task;
};

struct propBB_s
{
    vec4_u min;
    vec4_u max;
};

struct slinkdef_s
{
    slinkdef_s* next;
};

struct BgRoom_s
{
    unsigned int bgih;
    int room;
};

struct windowdef_s
{
    float left;
    float right;
    float top;
    float bottom;
    float width;
    float height;
    float midx;
    float midy;
    float halfwidth;
    float halfheight;
    float pixelaspect;
};

struct filterViewPostZWrite_s
{
    int atmosphere;
    float atmosRadius;
    float atmosHeight;
    float atmosDensity;
    float atmosAngleStartFade;
    float atmosAngleEndFade;
    float atmosMinFade;
    unsigned int atmosCubemap;
    float fogMin;
    float fogMax;
    float fogDensity;
    float fogAlpha;
    unsigned int fogCubemap;
};

struct filterViewPreUnfilteredAlpha_s
{
    float monochrome;
};

struct __declspec(align(4)) filterViewPreHdr_s
{
    float fogAccum;
    float fogNear;
    float fogFar;
    vec4_u fog;
    unsigned __int8 fogSky;
    unsigned __int8 fogJitter;
};

struct filterViewPostHdr_s
{
    float bloomBleed;
    float blur;
    float blurX;
    float blurY;
    float blurCamera;
    float blurRadial;
    float blurZoom;
    float digitalZoom;
    float dofAccum;
    float dof;
    float dofNear;
    float dofFar;
    float doubleVision;
    float edge;
    float emboss;
    float focus;
    float malfunction;
    float monochrome;
    float motion;
    float noise;
    float optimal;
    float screenShake;
    float sharpen;
    float sphere;
    float rainDensity;
    float rainMapCoords[4];
    float rainSpeed;
    float heatNear;
    float heatFar;
    float heatScale;
    float heatSpeed;
    float heat;
    unsigned int heatMap;
    unsigned int rainMap;
    vec3_u rainDir;
    vec3_u addHsl;
    vec3_u addRgb;
    vec3_u addScr;
    vec3_u mulRgb;
    vec4_u bloom;
    vec3_u perceptionBlobs[16];
    unsigned __int8 perceptionBlobNum;
    unsigned __int8 bloodMask;
    float health;
    double lastDamage;
    float stamina;
    double lastStaminaLoss;
};

struct filterViewPersistant
{
    vec3_u blurCameraDir;
};

struct __declspec(align(8)) filterView_s
{
    filterViewPostZWrite_s postZWrite;
    filterViewPreUnfilteredAlpha_s preUnfilteredAlpha;
    filterViewPreHdr_s preHdr;
    filterViewPostHdr_s postHdr;
    filterViewPersistant persistant;
};

enum Cam4x3Mode : __int32
{
    CAM_4X3_MODE_DISABLED = 0x0,
    CAM_4X3_MODE_LETTERBOX = 0x1,
    CAM_4X3_MODE_PAN_AND_SCAN = 0x2,
    CAM_4X3_MODE_STRETCH = 0x3,
};

struct viewdef_s
{
    slinkdef_s linkall;
    unsigned int flags;
    BgRoom_s room;
    BgRoomGroup_s roomGroup;
    vec3_u pos;
    vec3_u lastpos;
    vec3_u dir;
    vec3_u up;
    vec3_u right;
    float fovRadians;
    float fovDiameter;
    windowdef_s window;
    __declspec(align(16)) frustumdef_s frustum;
    filterView_s filters;
    Cam4x3Mode mode4x3;
    __declspec(align(8)) mtx_u perspmtx;
    mtx_u modelviewmtx;
    mtx_u invmodelviewmtx;
    mtx_u modelviewperspmtx;
    mtx_u invtransmodelviewpersmtx;
    int camprojmode;
    unsigned __int16 viewIdx;
    float ortho[2];
    float minz;
    float maxz;
    void(__cdecl* callback)(int, viewdef_s*);
    void* callbackData;
    HOM* hom;
    unsigned __int8 particleView;
    int sceneDescriptorIndex;
    struct opaqueProp* envMapProph;
    unsigned __int16 envMapSide;
    unsigned int envMapCurrentFrame;
    unsigned int envMapLastFrame;
    unsigned int envMapMaxTimeSlice;
    unsigned int envMapTimeSlice;
    unsigned __int16 envMapMaxRenderFreq;
    unsigned __int16 envMapRenderFreq;
    float envMapMaxDist;
    unsigned int envMapPriority;
};

struct obOCCObj_s
{
    unsigned int numPolygons;
    unsigned int numVertices;
    unsigned int vertsIndicesSize;
    vec4_u* vertices;
    int* indices;
};

struct __declspec(align(8)) SHomInternalArgs_s
{
    mtx_u rotTrans[1024];
    mtx_u modelView;
    mtx_u modelViewPers;
    propBB_s propBBs[1024];
    unsigned int propHandle[1024];
    unsigned int propsVisible[1024];
    unsigned int numProps;
    unsigned int numPropsVisible;
    viewdef_s* view;
    float* ooz;
    float* level1;
    float* level2;
    float* level3;
    float* level4;
    float* level5;
    unsigned int oozSize;
    unsigned int numOccluders;
    obOCCObj_s occluders[1024];
};

struct HOM
{
    mtx_u mvp;
    float ooz[65536];
    float level1[32768];
    float level2[8192];
    float level3[2048];
    float level4[512];
    float level5[128];
    int taskDone;
    unsigned int taskID;
    __declspec(align(16)) SHomInternalArgs_s internalArgs;
};

struct SHomArgs_s
{
    HOM* hom;
    SHomInternalArgs_s* internals;
};

struct SDataTransfer
{
    void* dest;
    void* src;
    unsigned int size;
};

union __declspec(align(16)) UTaskArgs
{
    SSkinningTaskArgs_s skinningArgs;
    SParticleTaskArgs_s particleArgs;
    STestTaskArgs_s testArgs;
    tm0vQue m0vArgs;
    tpakQue pakArgs;
    SLinetestArgs_s linetestArgs;
    SFrdLinetestArgs_s frdLinetestArgs;
    SNavRouteFindArgs_s navRouteArgs;
    SClothThreadArgs_s clothArgs;
    SABMTaskArgs_s abmArgs;
    SHomArgs_s homArgs;
    SDataTransfer dataTransfer;
    SPmeshTaskArgs_s pmeshArgs;
};


struct __declspec(align(8)) STask
{
    $286E6D0C4CB27190F2F64704DDAC088B ___u0;
    UTaskArgs args;
    char name[16];
    void(__cdecl* completedCallback)(void*);
    void* callbackArg;
    void* thandle;
};

enum ETaskFlags : __int32
{
    k_taskflag_none = 0x0,
    k_taskflag_dontWaitForEmptySlot = 0x1,
};

struct tsQueue
{
    DWORD  inUse;       // +0x00  1 = allocated
    DWORD  maxNumItems; // +0x04
    DWORD  itemSize;    // +0x08
    void* buffer;      // +0x0C
    DWORD  readPos;     // +0x10
    DWORD  writePos;    // +0x14
    DWORD  mutexId;     // +0x18  index into s_mutexSlots
    DWORD  condNotFull; // +0x1C  condition slot index — space available
    DWORD  condNotEmpty;// +0x20  condition slot index — item available
};

struct SEmbeddedResFileEntry
{
    unsigned int offsetAndType;
};

struct SEmbeddedResFileTemplate
{
    unsigned int offset;
    char* name;
};

struct embeddedPreloadEntry
{
    unsigned int keyOffset;
    unsigned int valueOffset;
    unsigned int metaOffset;
    unsigned int containerOffset;
    unsigned int gameLoadingFlagsModeEra;
    unsigned int gameLoadingFlagsMultiSizePlatform;
};

struct SEmbeddedResFileHeader
{
    unsigned __int8* resFileData;
    int resFileDataSize;
    char* keyPool;
    int keyPoolSize;
    char* strValuePool;
    int strValuePoolSize;
    SEmbeddedResFileEntry* entries;
    int numEntries;
    SEmbeddedResFileTemplate* templates;
    int numTemplates;
    embeddedPreloadEntry* preloadList;
    int preloadListSize;
    int animInfoOffset;
};

struct listState_s
{
    int maxSize;
    int count;
    unsigned __int8* items;
    unsigned int flags;
};

struct yy_buffer_state
{
    _iobuf* yy_input_file;
    char* yy_ch_buf;
    char* yy_buf_pos;
    unsigned int yy_buf_size;
    int yy_n_chars;
    int yy_is_our_buffer;
    int yy_is_interactive;
    int yy_at_bol;
    int yy_fill_buffer;
    int yy_buffer_status;
};

union rStateWriteCallback
{
    void(__cdecl* writeInt1)(int);
    void(__cdecl* writeInt2)(int, int);
    void(__cdecl* writeInt3)(int, int, int);
    void(__cdecl* writeInt4)(int, int, int, int);
    void(__cdecl* writeInt5)(int, int, int, int, int);
    void(__cdecl* writeInt6)(int, int, int, int, int, int);
    void(__cdecl* writeFloat1)(float);
    void(__cdecl* writeFloat2)(float, float);
    void(__cdecl* writeFloat3)(float, float, float);
    void(__cdecl* writeFloat4)(float, float, float, float);
};

struct rStateBase
{
    slinkdef_s link;
    rStateWriteCallback writeCB;
    unsigned __int16 vtype;
    unsigned __int8 numValues;
    unsigned __int8 pad;
};

struct rStateFloat4
{
    rStateBase base;
    float f[4];
};

struct rStateFloat1
{
    rStateBase base;
    float f[1];
};

struct rStateInt2
{
    rStateBase base;
    int i[2];
};

struct rStateInt1
{
    rStateBase base;
    int i[1];
};

struct rStateInt4
{
    rStateBase base;
    int i[4];
};

struct rStateFloat2
{
    rStateBase base;
    float f[2];
};

struct rStateInt3
{
    rStateBase base;
    int i[3];
};

struct rStateValueStore
{
    rStateInt1 facecull;
    rStateInt1 shade;
    rStateInt1 blend;
    rStateInt4 colourmask;
    rStateInt1 scissortest;
    rStateInt1 depthtest;
    rStateInt1 depthwrite;
    rStateFloat2 depthbias;
    rStateFloat2 depthrange;
    rStateInt1 depthfunc;
    rStateInt2 alphatest;
    rStateInt1 stenciltest;
    rStateInt3 stencilfunc;
    rStateInt3 stencilop;
    rStateInt3 stencilop2;
    rStateInt1 wireframe;
    rStateInt1 multisample;
    rStateInt1 alphaToCoverage;
    rStateFloat4 clearcolour;
    rStateFloat1 cleardepth;
    rStateFloat1 pointsize;
    rStateFloat1 linewidth;
    rStateInt4 viewport;
    rStateInt4 scissor;
    rStateInt3 texture[16];
    rStateInt1 vertexprogenable;
    rStateInt1 fragmentprogenable;
    rStateInt1 indexbuffer;
    rStateInt1 vertexbuffer;
};

#include <d3d9.h>

union $46C50913BD7678ED09F932FB39EDE5EA
{
    IDirect3DTexture9* dxtex;
    IDirect3DCubeTexture9* dxcube;
    IDirect3DVolumeTexture9* dxvol;
};

/* 17287 */
struct texdefHW
{
    $46C50913BD7678ED09F932FB39EDE5EA ___u0;
    void* address;
    unsigned int vmemHandle;
    unsigned int texHandle;
    __int16 texLevel;
    __int16 maxMipmapLevel;
};

enum TexType : __int32
{
    TEX_TYPE_UNKNOWN = 0x0,
    TEX_TYPE_1D = 0x1,
    TEX_TYPE_2D = 0x2,
    TEX_TYPE_RECTANGLE = 0x3,
    TEX_TYPE_CUBE = 0x4,
    TEX_TYPE_3D = 0x5,
    TEX_TYPE_HDR = 0x6,
};

struct texaniminfo_s
{
    unsigned int curframe;
    unsigned int baseframe;
    unsigned int time;
    unsigned int timeperframeUS;
};

struct texdef_s
{
    int id;
    unsigned int streamflags;
    unsigned int flags;
    unsigned __int16 count;
    __int16 width;
    __int16 height;
    __int16 depth;
    TexType type;
    unsigned int ftHandle;
    unsigned int hw[3];
    int hwFrameNeeded[3];
    int hwFileSize[3];
    float minLod;
    unsigned int nextframe;
    texaniminfo_s* animinfo;
    float lightmapScale;
};

struct mattexslotdef_s
{
    unsigned int ID;
    unsigned int srcOffset;
    unsigned int flags;
    unsigned int pad1;
};

struct texExpression_s
{
    unsigned int tokenCount;
    unsigned int exp[];
};

struct texExpressionSet_s
{
    texExpression_s* offsetU;
    texExpression_s* offsetV;
    texExpression_s* repeatU;
    texExpression_s* repeatV;
    texExpression_s* rotateUV;
};

struct expSetPtrDef
{
    texExpressionSet_s* p;
};

struct __declspec(align(8)) extraValue_s
{
    char8_s nickName;
    unsigned int value;
};

struct matinfodef_s
{
    mattexslotdef_s textureSlot[10];
    expSetPtrDef expSetPtrs[4];
    extraValue_s* extraValues;
    texExpression_s* incanGlowExp;
    texExpression_s* colourGainExp;
    unsigned int flags;
    unsigned int padding;
    unsigned __int8 shaderId;
    unsigned __int8 blendType;
    unsigned __int8 numExtraValues;
    unsigned __int8 alphaCutout;
    float specularPower;
    float paralaxDepth;
    float envMapIntensity;
    float envMapFresnel;
    float incanGlow;
    float detailTexScale[2];
    float blendTexScale[2];
    float detailMapFadeDist;
    float beamDefaults[4];
    float terrainTiles[5];
    float waterSettings[2];
    unsigned int ambientRGB;
};

struct pmatinfodef_s
{
    char* name;
    unsigned int pmh;
    unsigned int flags;
};

struct lightcreatedef_s
{
    vec3_u pos;
    vec3_u dir;
    float col[3];
    int shaderID;
    unsigned int _lightType;
    float angle;
    float intensity;
    texExpression_s* intensityExp;
    float exponent;
    float range;
};

struct ellipsoiddef_s
{
    vec3_u centre;
    vec3_u r;
    vec3_u s;
    vec3_u t;
};

struct colpolydef_s
{
    unsigned __int16 surf;
    unsigned __int16 vtx[3];
    unsigned __int8 xyz;
    unsigned __int8 glsid;
    unsigned __int8 hardedge;
    unsigned __int8 matidx;
};

struct coltreenodedef_s
{
    unsigned __int16 val0;
    unsigned __int16 val1;
    unsigned __int8 cbb[2][3];
};

struct collisionPolyInfo_s
{
    int numPolys;
    colpolydef_s* polys;
    int novodexIdx;
};

struct collisiondatadef_s
{
    aabbdef_u bb;
    int numtotalpolys;
    int numphysicspolys;
    colpolydef_s* polys;
    coltreenodedef_s* coltree;
    collisionPolyInfo_s* infos[3];
};

struct partdef_s
{
    unsigned int flags;
    unsigned int switchmask;
    __int16 matrixnum;
    unsigned __int16 nameOffset;
    unsigned __int16 numlights;
    unsigned __int16 lodMergeData;
    __int16 parent;
    __int16 sibling;
    __int16 child;
    unsigned __int8 pad;
    unsigned __int8 lodLevel;
    float nearDist;
    float farDist;
    lightcreatedef_s* lights;
    aabbdef_u aabb;
    ellipsoiddef_s ellipsoid;
    collisiondatadef_s* collisiondata;
    void* hwdata;
};

struct obDofInfo_s
{
    vec3_u pos;
    vec3_u rot;
    unsigned __int16 partNum;
    unsigned __int16 nameOffset;
};

struct obRefLocator_s
{
    vec3_u pos;
    vec3_u normal;
    unsigned __int16 nameOffset;
    unsigned __int16 refNameOffset;
    unsigned __int16 refTypeOffset;
    unsigned __int16 pad;
};

struct obRefLocators_s
{
    unsigned int numLocators;
    obRefLocator_s refLocs[];
};

struct $E960916A9D7BAC2817DEAEBE0A415BD3
{
    unsigned __int8 type;
    char pad[3];
    int boneIdx;
};

union $22687E0FDC4A35DBA0A0FDC6EF4CDBB2
{
    char shapeType[8];
    $E960916A9D7BAC2817DEAEBE0A415BD3 __s1;
};

struct obRBInfo_s
{
    vec3_u pos;
    vec3_u scale;
    vec3_u rot;
    $22687E0FDC4A35DBA0A0FDC6EF4CDBB2 ___u3;
    unsigned int pmh;
    unsigned __int16 partNum;
    unsigned __int16 nameOffset;
    unsigned __int8* novodexCylinderMesh;
};

struct glassPoint_s
{
    vec3_u pos;
    vec4_u col;
    float uv[2];
};

struct glassPoly_s
{
    vec3_u norm;
    int numPoints;
    glassPoint_s* points;
};

struct glassDef_s
{
    unsigned __int16 partnum;
    unsigned __int16 glassFlags;
    unsigned int material;
    int numPolys;
    glassPoly_s* polys;
};

struct physicsInfo_bgmap
{
    int N;
    unsigned int* indices;
    unsigned __int16* parts;
};

struct physicsInfo_s
{
    physicsInfo_bgmap* maps[3];
};

struct ClothMat_s
{
    float unused1;
    float thickness;
    float unused2;
    float damping;
    float stretchStiffness;
    float bendingStiffness;
    unsigned int unused3;
};

struct clothAttachInfo_s
{
    unsigned __int16 vert;
    unsigned __int16 pad;
    unsigned __int16 matrixIdx[4];
    float weight[4];
};

struct clothVertInfo_s
{
    float attachWeight;
    unsigned __int16 matrixIdx[4];
    float weight[4];
};

struct obClothGeom_s
{
    vec3_u pos;
    vec3_u scale;
    vec3_u rot;
    char shapeType[8];
    char boundToBone[32];
};

struct clothDef_s
{
    ClothMat_s mat;
    __int16 NOT_USED;
    __int16 partIdx;
    unsigned __int16 numVerts;
    unsigned __int16 numTris;
    unsigned __int16 numAttachedVerts;
    unsigned __int16 numRenderedVerts;
    unsigned int flags;
    unsigned __int16 numGeoms;
    unsigned __int16 pad;
    vec3_u* verts;
    unsigned __int16* map;
    unsigned __int16* tris;
    clothAttachInfo_s* attachedVerts;
    clothVertInfo_s* vertInfos;
    obClothGeom_s* geoms;
};

struct obPJInfo_s
{
    vec3_u pos;
    vec3_u rot;
    unsigned __int16 nameOffset;
    unsigned __int16 part1_nameOffset;
    unsigned __int16 part2_nameOffset;
    unsigned __int16 type_nameOffset;
    float lowLimit;
    float highLimit;
    unsigned int flags;
};

struct particleRampCoord_s
{
    float time;
    float intensity;
};

struct particleRamp_s
{
    unsigned int numCoords;
    particleRampCoord_s coords[5];
};

struct ParticleMaterial_s
{
    unsigned int textureColour;
    unsigned int textureNormal;
    unsigned int textureSpecular;
};

struct particleSystem_s
{
    union unnamed_tag
    {
        ParticleMaterial_s particleMaterial;
        unsigned int particleGeom;
        unsigned int material;
    };

    const char* particleName;
    unsigned int nameHash;
    unsigned int particleType;
    unsigned __int16 matrixNum;
    unsigned __int16 particleColourFormat;
    unsigned __int8 particleDecalLimit;
    unsigned __int8 particleDecalMaterialNum;
    unsigned __int8 particleTileCount[2];
    float particleTileSpeed;
    float particleLife;
    float particleLifeRandom;
    float particleRotation;
    float particleRotationRandom;
    float particleRotationSpeed;
    float particleRotationSpeedRandom;
    float particleMass;
    float particleRestitution;
    float particleRestitutionRandom;
    float particleCollisionSpread;
    float particleLuminosity;
    float particleLuminosityRadius;
    float particleRefraction;
    float particleBloom;
    float particleBlur;
    float particleDecalSize;
    float particleDecalSizeRandom;
    float particleSizeLimit;
    float particleTexCoords[3][4];
    particleRamp_s particleAdditiveRamp;
    particleRamp_s particleAspectRamp;
    particleRamp_s particleColourRamp[4];
    particleRamp_s particleColourRandomRamp[4];
    particleRamp_s particleSizeRamp;
    particleRamp_s particleSizeRandomRamp;
    particleRamp_s particleDirectionAlignRamp;
    unsigned int particleDecalMaterials[4];
    particleSystem_s::unnamed_tag shading;
    float particleLeaderLife;
    float particleLeaderLifeRandom;
    float particleLeaderMass;
    float particleLeaderSpeedScale;
    float particleLeaderSpeedScaleRandom;
    unsigned int particleFlags;
    char16_s particleChildName;
    unsigned int particleChildBirthFrequency;
    float particleChildBirthTime;
    float particleChildBirthTimeRandom;
    unsigned int emitterType;
    unsigned int emitterFlags;
    float emitterLife;
    float emitterDelay;
    float emitterRate;
    float emitterSpreadMin;
    float emitterSpreadMax;
    float emitterSpreadRandom;
    float emitterSpeed;
    float emitterSpeedRandom;
    float emitterDuration;
    float emitterDurationRandom;
    float emitterInterval;
    float emitterIntervalRandom;
    float emitterRotationRandom;
    float emitterStartDistance;
    float emitterStartDistanceRandom;
    float emitterPosition[3];
    float emitterRotation[3];
    float emitterNormal[3];
    float emitterSize[3];
    unsigned int emitterSound;
    unsigned int emitterSeed;
    unsigned int emitterClusters;
    float emitterClusterSpreadMin;
    float emitterClusterSpreadMax;
    float emitterClusterSpreadRandom;
    float emitterClusterRadius;
    float forceTurbulanceAmplitude[3];
    float forceTurbulanceFrequency[3];
    float forceGravity;
    float forceAir;
    unsigned int particleDecalType;
};

struct particleSystemPtr64_s
{
    particleSystem_s* ptr;
};

struct __declspec(align(8)) particleEffect_s
{
    char16_s name;
    int numSystems;
    particleSystemPtr64_s systems[12];
};

struct facemePoly_s
{
    int numvtx;
    vec3_u centre;
    float col[4];
    vec4_u* vtx;
};

struct facemeDef_s
{
    unsigned __int16 partnum;
    unsigned __int16 numpolys;
    unsigned int material;
    unsigned int facemeflags;
    facemePoly_s* poly;
};

struct obLadderInfo_s
{
    vec3_u top;
    vec3_u bottom;
    float width;
    vec3_u normal;
};

union $CD6A3D30F2A15537B9C0E336C988B538
{
    unsigned int offset;
    unsigned int constColour;
    unsigned __int8 constColourABGR[4];
};

struct streamformat_s
{
    unsigned __int32 type : 5;
    unsigned __int32 elements : 3;
    unsigned __int32 numX : 12;
    unsigned __int32 numY : 4;
    unsigned __int32 numZ : 4;
    unsigned __int32 bDuped : 1;
    unsigned __int32 quantisedRange : 1;
    unsigned __int32 pad2 : 2;
};

struct istreaminfo
{
    char8_s type;
    streamformat_s format;
    $CD6A3D30F2A15537B9C0E336C988B538 ___u2;
};

struct istreamdef
{
    unsigned int num;
    istreaminfo* info;
    void* data;
    unsigned int buffer;
    unsigned int stride;
};

struct istreamsize
{
    unsigned int totistreams;
    unsigned int streamsize;
};

struct streamdef_s
{
    char8_s type;
    streamformat_s format;
    unsigned int buffer;
    unsigned int offset;
    void* data;
};

struct streamArray_s
{
    unsigned int numIStreams;
    unsigned int numUniform;
    unsigned int* uniformSizes;
    istreamsize* IStreamSizes;
    streamdef_s* pUniform;
    istreamdef* pIStreams;
};

struct glow_s
{
    int shaderID;
    unsigned int flags;
    unsigned int partnum;
    vec3_u position;
    float width;
    float height;
};

struct obdef_s
{
    unsigned int flags;
    float obScale;
    __int16 numMtxs;
    unsigned __int16 numFaceme;
    unsigned __int16 numParts;
    unsigned __int16 numDofs;
    unsigned __int16 numRBs;
    unsigned __int16 numOccs;
    unsigned __int16 numGlass;
    unsigned __int16 numGlows;
    unsigned __int16 numCloth;
    unsigned __int16 numPhysicsJoints;
    unsigned __int16 numParticleEffects;
    unsigned __int16 numParticleSystems;
    unsigned __int16 numLadders;
    unsigned __int16 numMaterials;
    matinfodef_s* matInfos;
    char* texturesrcnames;
    pmatinfodef_s* pmatInfos;
    obSkeletonDef_s* skeleton;
    aabbdef_u* boneBB;
    aabbdef_u* boneBBInBoneSpace;
    partdef_s* parts;
    obDofInfo_s* dofs;
    obRefLocators_s* reflocs;
    obRBInfo_s* rbs;
    obOCCObj_s* occluders;
    glassDef_s* glass;
    physicsInfo_s* physicsInfo;
    clothDef_s* cloth;
    obPJInfo_s* physicsjoints;
    particleEffect_s* particleEffects;
    particleSystem_s* particleSystems;
    facemeDef_s* faceme;
    obLadderInfo_s* ladders;
    streamArray_s* sharedStreams;
    glow_s* glows;
};

struct obHandleDef
{
    void* filebase;
    obdef_s* obPtr;
    unsigned int hmem;
    unsigned int ftHandle;
    unsigned __int16 refCount;
    unsigned __int16 lockCount;
    unsigned __int16 obFlags;
    unsigned __int8 state;
    unsigned __int8 shadow;
};

struct PBuffer_s
{
    unsigned __int8 valid;
    unsigned __int8 bFloat;
    int width;
    int height;
    int bpc;
    unsigned int colorTexture;
    unsigned int depthTexture;
};

struct __declspec(align(4)) electricBit_s
{
    unsigned int AssociatedLineHandle;
    vec3_u vertices[16];
    vec3_u v1;
    vec3_u v2;
    vec3_u v3;
    vec3_u v4;
    vec3_u old_v1;
    vec3_u axis;
    float line_position;
    float offset;
    float angle;
    float length;
    unsigned __int8 samples;
    unsigned __int8 set_up;
};

struct electricLine_s
{
    unsigned int* refHandle;
    vec3_u vertices[128];
    vec3_u v1;
    vec3_u v2;
    vec3_u v3;
    vec3_u v4;
    vec4_u line_colour;
    vec4_u glow_colour;
    float time;
    float life;
    float offset;
    float frequency;
    float amplitude;
    float length;
    float update_time;
    unsigned __int8 samples;
    unsigned __int8 type;
    unsigned __int8 used;
    electricBit_s bits[4];
};

struct electricSphereLine_s
{
    unsigned int lineHandle;
    float v1angle1;
    float v1angle2;
    float v1angle1Speed;
    float v1angle2Speed;
    float v2angle1;
    float v2angle2;
    float v2angle1Speed;
    float v2angle2Speed;
    float directionTime;
    float feelerTime;
};
struct __declspec(align(4)) electricSphere_s
{
    unsigned int* refHandle;
    electricSphereLine_s lines[8];
    vec3_u pos;
    float radius;
    unsigned __int8 used;
};

struct lightningPoint_s
{
    vec3_u p;
    float offsetY;
    float offsetX;
};

struct lightning_s
{
    vec3_u controlPoints[21];
    vec3_u interpControlPoints[21];
    vec3_u pointList[140];
    vec3_u oldPointList[140];
    lightningPoint_s displacedPointList[140];
    vec3_u position;
    vec3_u direction;
    float spawnTime;
    float currentTime;
    float life;
    float lifeSpan;
    float lifeScale;
    float radiusScale;
    float rayScale;
    unsigned __int8 bUsed;
    unsigned __int8 bFirstInit;
    unsigned __int8 bDisable;
    vec3_u color;
    unsigned int* refHandle;
    unsigned int fxOwner;
};

struct slinklistdef_s
{
    slinkdef_s* head;
    int offset;
};

struct obStreamDef
{
    unsigned int obh;
    int streamHandle;
    void* tempPhysicsBuffer;
};

enum MenuBlockType : __int32
{
    MENU_BLK_UNUSED = 0x0,
    MENU_BLK_HEAD = 0x1,
    MENU_BLK_TEXT = 0x2,
    MENU_BLK_TOGGLE = 0x3,
    MENU_BLK_SUBMENU = 0x4,
    MENU_BLK_POSITION = 0x5,
    MENU_BLK_SELECT = 0x6,
    MENU_BLK_OPTIONS = 0x7,
    MENU_BLK_SLIDER = 0x8,
    MENU_BLK_COLOUR_PICKER = 0x9,
    MENU_BLK_BACK = 0xA,
    MENU_BLK_FONT = 0xB,
    MENU_BLK_CALLBACK = 0xC,
    MENU_BLK_CONDITION = 0xD,
    MENU_BLK_UPDATEFUNCTION = 0xE,
};

struct MenuLink_s
{
    void* Next;
    MenuBlockType Type;
    int(__cdecl* fn)(unsigned int, int);
    int user;
    unsigned __int32 : 30;
    unsigned __int32 visible : 1;
    unsigned __int32 selectable : 1;
    int Flags;
    float cbtop;
    float cbleft;
};

struct MenuText_s
{
    void* Next;
    MenuBlockType Type;
    int(__cdecl* fn)(unsigned int, int);
    int user;
    unsigned __int32 : 30;
    unsigned __int32 visible : 1;
    unsigned __int32 selectable : 1;
    int Flags;
    float cbtop;
    float cbleft;
    char* str;
};

enum MenuToggleType : __int32
{
    MENU_TOGGLE = 0x0,
    MENU_TOGGLE_TABS = 0x1,
};

struct MenuToggle_s
{
    void* Next;
    MenuBlockType Type;
    int(__cdecl* fn)(unsigned int, int);
    int user;
    unsigned __int32 : 30;
    unsigned __int32 visible : 1;
    unsigned __int32 selectable : 1;
    int Flags;
    float cbtop;
    float cbleft;
    MenuToggleType type;
    int size;
    int bit;
    void* var;
    char* str;
    char* str1;
    char* str0;
};

struct MenuSubMenu_s
{
    void* Next;
    MenuBlockType Type;
    int(__cdecl* fn)(unsigned int, int);
    int user;
    unsigned __int32 : 30;
    unsigned __int32 visible : 1;
    unsigned __int32 selectable : 1;
    int Flags;
    float cbtop;
    float cbleft;
    unsigned int* handle;
    char* str;
};

struct MenuPosition_s
{
    void* Next;
    MenuBlockType Type;
    int(__cdecl* fn)(unsigned int, int);
    int user;
    unsigned __int32 : 30;
    unsigned __int32 visible : 1;
    unsigned __int32 selectable : 1;
    int Flags;
    float cbtop;
    float cbleft;
    float x;
    float y;
};

struct MenuSelect_s
{
    void* Next;
    MenuBlockType Type;
    int(__cdecl* fn)(unsigned int, int);
    int user;
    unsigned __int32 : 30;
    unsigned __int32 visible : 1;
    unsigned __int32 selectable : 1;
    int Flags;
    float cbtop;
    float cbleft;
    int* var;
    int val;
    char* str;
    unsigned __int8 ownsStr;
};

struct MenuOptions_s
{
    void* Next;
    MenuBlockType Type;
    int(__cdecl* fn)(unsigned int, int);
    int user;
    unsigned __int32 : 30;
    unsigned __int32 visible : 1;
    unsigned __int32 selectable : 1;
    int Flags;
    float cbtop;
    float cbleft;
    int num;
    int* var;
    int size;
    void* val;
    char* str;
    char** strs;
    int cursel;
};

struct $508F737789F53E78B141F7F3449C5653
{
    float* var;
    float min;
    float max;
    float step;
    float marker;
};

struct $E1DE2642C0D558F7CE3719AB2691A9A9
{
    int* var;
    int min;
    int max;
    int step;
};

struct $446CAF610B0D0C6C3141BA2C4FCECB15
{
    float* var;
    float min;
    float max;
    float step;
};

union $66EF2A6C2906D70D5223C115646B9A39
{
    $446CAF610B0D0C6C3141BA2C4FCECB15 Float;
    $E1DE2642C0D558F7CE3719AB2691A9A9 Int;
    $508F737789F53E78B141F7F3449C5653 Marked;
};

enum MenuSliderType : __int32
{
    MENU_SLIDER_FLOAT = 0x0,
    MENU_SLIDER_INT = 0x1,
    MENU_SLIDER_MARKER = 0x2,
    MENU_SLIDER_TABS_FLOAT = 0x3,
};

struct MenuSlider_s
{
    void* Next;
    MenuBlockType Type;
    int(__cdecl* fn)(unsigned int, int);
    int user;
    unsigned __int32 : 30;
    unsigned __int32 visible : 1;
    unsigned __int32 selectable : 1;
    int Flags;
    float cbtop;
    float cbleft;
    MenuSliderType type;
    char* str;
    float w;
    $66EF2A6C2906D70D5223C115646B9A39 ___u12;
};

struct MenuColPicker_s
{
    void* Next;
    MenuBlockType Type;
    int(__cdecl* fn)(unsigned int, int);
    int user;
    unsigned __int32 : 30;
    unsigned __int32 visible : 1;
    unsigned __int32 selectable : 1;
    int Flags;
    float cbtop;
    float cbleft;
    char* str;
    vec3_u* var;
    unsigned __int8 active;
};

struct MenuBack_s
{
    void* Next;
    MenuBlockType Type;
    int(__cdecl* fn)(unsigned int, int);
    int user;
    unsigned __int32 : 30;
    unsigned __int32 visible : 1;
    unsigned __int32 selectable : 1;
    int Flags;
    float cbtop;
    float cbleft;
    char* str;
};

struct MenuFont_s
{
    void* Next;
    MenuBlockType Type;
    int(__cdecl* fn)(unsigned int, int);
    int user;
    unsigned __int32 : 30;
    unsigned __int32 visible : 1;
    unsigned __int32 selectable : 1;
    int Flags;
    float cbtop;
    float cbleft;
    unsigned int fh;
    float scale;
};

struct MenuCallback_s
{
    void* Next;
    MenuBlockType Type;
    int(__cdecl* fn)(unsigned int, int);
    int user;
    unsigned __int32 : 30;
    unsigned __int32 visible : 1;
    unsigned __int32 selectable : 1;
    int Flags;
    float cbtop;
    float cbleft;
};

enum MenuBlockCondition : __int32
{
    MENU_CONDITION_EQUAL = 0x0,
    MENU_CONDITION_NOT_EQUAL = 0x1,
    MENU_CONDITION_LESS = 0x2,
    MENU_CONDITION_GREATER = 0x3,
};

struct MenuCondition_s
{
    void* Next;
    MenuBlockType Type;
    int(__cdecl* fn)(unsigned int, int);
    int user;
    unsigned __int32 : 30;
    unsigned __int32 visible : 1;
    unsigned __int32 selectable : 1;
    int Flags;
    float cbtop;
    float cbleft;
    MenuBlockCondition condition;
    int* var;
    int val;
};

struct MenuUpdate_s
{
    void* Next;
    MenuBlockType Type;
    int(__cdecl* fn)(unsigned int, int);
    int user;
    unsigned __int32 : 30;
    unsigned __int32 visible : 1;
    unsigned __int32 selectable : 1;
    int Flags;
    float cbtop;
    float cbleft;
    void(__cdecl* ufn)();
};

struct FontColour_s
{
    float r;
    float g;
    float b;
    float a;
};

struct MenuHead_s
{
    void* Next;
    MenuBlockType Type;
    int(__cdecl* fn)(unsigned int, int);
    int user;
    unsigned __int32 : 30;
    unsigned __int32 visible : 1;
    unsigned __int32 selectable : 1;
    int Flags;
    float cbtop;
    float cbleft;
    int CurSelection;
    MenuUnion_s* Current;
    MenuUnion_s* LastEntry;
    enum : __int32
    {
        FONT_WINDOW_BORDER_NONE = 0x0,
        FONT_WINDOW_BORDER_SOLID = 0x1,
        FONT_WINDOW_BORDER_OUTSET = 0x2,
        FONT_WINDOW_BORDER_INSET = 0x3,
        FONT_WINDOW_BORDER_DUMMY = 0xFFFFFFFF,
    } style;
    unsigned int defH;
    float defscale;
    float paddingX;
    float paddingY;
    float borderX;
    float borderY;
    FontColour_s col[12];
    float x;
    float y;
    float w;
    float h;
    int(__cdecl* closeCB)(unsigned int, int);
    int closeCBuser;
    unsigned __int32 : 30;
    unsigned __int32 creation : 1;
    unsigned __int32 dynamicheight : 1;
};

struct __declspec(align(2)) flare_s
{
    vec3_u position;
    vec4_u colour;
    float radius;
    float time;
    float lifeTime;
    float cutoffDistance;
    unsigned __int8 additive;
    unsigned __int8 normalMapped;
    unsigned __int8 used;
};

struct __declspec(align(4)) explosion_s
{
    vec3_u position;
    float scale;
    float time;
    float lifeTime;
    float cutoffDistance;
    unsigned __int8 used;
};

union __declspec(align(4)) MenuUnion_s
{
    MenuLink_s Link;
    MenuHead_s Head;
    MenuText_s Text;
    MenuToggle_s Toggle;
    MenuSubMenu_s SubMenu;
    MenuPosition_s Position;
    MenuSelect_s Select;
    MenuOptions_s Options;
    MenuSlider_s Slider;
    MenuColPicker_s Picker;
    MenuBack_s Back;
    MenuFont_s Font;
    MenuCallback_s Callback;
    MenuCondition_s Condition;
    MenuUpdate_s UpdateFN;
};

enum ShaderUVSetFlags : __int32
{
    SHUVFLG_NONE = 0x0,
    SHUVFLG_FORCE_V3 = 0x1,
    SHUVFLG_FORCE_V4 = 0x2,
};

struct ShaderUVSetInfo
{
    char8_s name;
    __int16 set;
    unsigned __int16 groupMask;
    ShaderUVSetFlags flags;
    ShaderUVSetInfo* next;
    unsigned int pad32;
};

struct rStateBlock
{
    slinklistdef_s changed;
    rStateValueStore values;
};

struct SCloudLayer_s
{
    unsigned int m_bEnable;
    unsigned int m_bInitialised;
    float m_noiseOctaveWeights[8];
    float m_noiseOctaveNormWeights[8];
    float m_noiseOctaveEvolFrequencies[8];
    float m_noiseOctaveBlendsTemp[8];
    float m_noiseOctaveBlends[8];
    float m_cloudCover;
    float m_cloudSharpness;
    float m_cloudNoiseTexTilingScale;
    float m_cloudHalfHeight;
    vec3_u m_cloudDarkColor;
    vec3_u m_cloudLightColor;
    float m_cloudMaxLighting;
    float m_cloudMinLighting;
    float m_lightScattering;
    float m_lightrayStepLength;
    float m_windSpeed;
    float m_windAngleFromXAxis;
    float m_windOffset[2];
    float m_windAngle;
    float m_planeSizeScale;
    float m_planeAltitude;
    vec3_u m_planeCenter;
    vec3_u m_sunPosForCloud;
    float m_sunDistance;
};

struct $C4D2F73DBFBDCD0A2B21F4B1BEF68177
{
    unsigned __int8 b;
    unsigned __int8 g;
    unsigned __int8 r;
    unsigned __int8 a;
};

union PlatformColour_u
{
    $C4D2F73DBFBDCD0A2B21F4B1BEF68177 __s0;
    unsigned int col;
};

union rStateFloatIntUnion
{
    int i;
    float f;
};

struct rStateBaseEx
{
    rStateBase base;
    rStateFloatIntUnion args[6];
};

struct ShaderTreeMaterialSamplerMap
{
    char** maps;
    unsigned __int16 numMaps;
    unsigned __int16 numMapsReserved;
    unsigned __int8 mapShaderIds[];
};


union $21D7E06F25E7019387B855BD7632CB52
{
    ShaderTreeMaterialSamplerMap* materialSamplerMap;
    unsigned int temp;
};

enum ShaderBindingID : __int32
{
    SHBIND_INVALID = 0x0,
    SHBIND_POSITION = 0x1,
    SHBIND_COLOR = 0x2,
    SHBIND_TEXCOORD = 0x3,
    SHBIND_NORMAL = 0x4,
    SHBIND_WPOS = 0x5,
    SHBIND_DEPTH = 0x6,
};

struct ShaderBindingType
{
    const char* name;
    ShaderBindingID id;
    unsigned __int16 nameLen;
    unsigned __int16 max;
};

struct $EAE1683833BC22952A66EA80C42A7D11
{
    float e[3][3];
};

struct $2373B7710E3C5480B3677736D0D41D52
{
    float e[3][4];
};

struct $4D23C2AEF2E29ACA8DADA0CCAECFA2AD
{
    float e[4][3];
};

struct $99B5B1C8A1B6A4D7BF18EE8273F7C2E4
{
    float e[4][4];
};

struct $4AB6D850D195388B5F18A2AF9B3D520D
{
    float x;
};

union ShaderVariableData
{
    $4AB6D850D195388B5F18A2AF9B3D520D float1;
    $38C3DEC81229B66F67FB6D350D75FF5A float2;
    $393C16A032292777F0C3725FFB2C0008 float3;
    $91D1B2149FAC90180ECB9AC277F76009 float4;
    $EAE1683833BC22952A66EA80C42A7D11 float3x3;
    $2373B7710E3C5480B3677736D0D41D52 float3x4;
    $4D23C2AEF2E29ACA8DADA0CCAECFA2AD float4x3;
    $99B5B1C8A1B6A4D7BF18EE8273F7C2E4 float4x4;
    unsigned int tex;
    mtx_u matrix4x4;
    unsigned __int64 u64s[1];
    unsigned int u32s[1];
};

struct ShaderBinding
{
    ShaderBindingType* type;
    int index;
};

enum ShaderVariableID : __int32
{
    SHVAR_INVALID = 0x0,
    SHVAR_HALF = 0x1,
    SHVAR_HALF2 = 0x2,
    SHVAR_HALF3 = 0x3,
    SHVAR_HALF4 = 0x4,
    SHVAR_FLOAT = 0x5,
    SHVAR_FLOAT2 = 0x6,
    SHVAR_FLOAT3 = 0x7,
    SHVAR_FLOAT4 = 0x8,
    SHVAR_HALF3x3 = 0x9,
    SHVAR_HALF3x4 = 0xA,
    SHVAR_HALF4x3 = 0xB,
    SHVAR_HALF4x4 = 0xC,
    SHVAR_FLOAT3x3 = 0xD,
    SHVAR_FLOAT3x4 = 0xE,
    SHVAR_FLOAT4x3 = 0xF,
    SHVAR_FLOAT4x4 = 0x10,
    SHVAR_SAMPLER_1D = 0x11,
    SHVAR_SAMPLER_2D = 0x12,
    SHVAR_SAMPLER_3D = 0x13,
    SHVAR_SAMPLER_CUBE = 0x14,
    SHVAR_SAMPLER_RECT = 0x15,
};

enum ShaderCommandID : __int32
{
    SHCMD_INVALID = 0x0,
    SHCMD_INCLUDE = 0x1,
    SHCMD_REF = 0x2,
    SHCMD_PTRREF = 0x3,
    SHCMD_END = 0x4,
    SHCMD_ECHO = 0x5,
    SHCMD_DEFINE = 0x6,
    SHCMD_GLOBAL = 0x7,
    SHCMD_PUSH = 0x8,
    SHCMD_POP = 0x9,
    SHCMD_RENDERLEVEL = 0xA,
    SHCMD_MATERIAL = 0xB,
    SHCMD_SHADER = 0xC,
    SHCMD_PASS = 0xD,
    SHCMD_ITERATION = 0xE,
    SHCMD_PASSES = 0xF,
    SHCMD_PERLIGHT = 0x10,
    SHCMD_OPTION = 0x11,
    SHCMD_STATE = 0x12,
    SHCMD_TARGET = 0x13,
    SHCMD_FILE = 0x14,
    SHCMD_VERTEX = 0x15,
    SHCMD_PIXEL = 0x16,
    SHCMD_FRAGMENT = 0x17,
    SHCMD_IF = 0x18,
    SHCMD_ELSE = 0x19,
    SHCMD_INPUT = 0x1A,
    SHCMD_TRANS = 0x1B,
    SHCMD_OUTPUT = 0x1C,
    SHCMD_UNIFORM = 0x1D,
    SHCMD_CODE = 0x1E,
    SHCMD_VAR_NOT_FOUND = 0x1F,
};

enum ShaderVariableFlags : __int32
{
    SHVARFLG_NONE = 0x0,
    SHVARFLG_TEXTURE = 0x1,
    SHVARFLG_TEXF3 = 0x2,
};

struct ShaderVariableType
{
    unsigned int nameLen;
    const char* name;
    ShaderVariableID id;
    ShaderVariableFlags flags;
    int datasize;
    int dim;
};

struct ShaderVariable
{
    ShaderBinding binding;
    ShaderVariableData* data;
    ShaderVariableType* type;
    const char* name;
    ShaderCommandID scope;
    __int16 arraysize;
    unsigned __int8 nameLen;
    unsigned __int8 dirty;
};

struct ShaderVarRef
{
    ShaderVariable* var;
};

struct ShaderUniform
{
    ShaderVarRef* uniform;
    unsigned __int16 numSampler;
    unsigned __int16 numUniform;
};

struct ShaderStream
{
    ShaderVariableType* type;
    const char* name;
    ShaderCommandID scope;
    unsigned __int8 nameLen;
    unsigned __int8 isSkinningBuffer;
    unsigned __int8 iStream;
    unsigned __int8 subStream;
    ShaderBinding binding;
};

struct ShaderVarying
{
    ShaderStream* stream;
    int num;
};

struct ShaderDefine
{
    const char* key;
    const char* value;
    int len;
};

struct ShaderDefineSetInfo
{
    unsigned __int16 num;
    unsigned __int16 max;
    ShaderDefine define[];
};

struct ShaderDefineSet
{
    ShaderDefineSetInfo* info;
    unsigned __int8 evals[];
};

union $CBA044050C21FFCF316D9DA10CB8229D
{
    IDirect3DPixelShader9* pixel;
    IDirect3DVertexShader9* vertex;
};

enum ShaderProgramType : __int32
{
    SHADER_TYPE_UNKNOWN = 0x0,
    SHADER_TYPE_VERTEX = 0x1,
    SHADER_TYPE_PIXEL = 0x2,
};

struct ShaderProgram
{
    ShaderProgramType type;
    $CBA044050C21FFCF316D9DA10CB8229D hw;
    void* ctable; //swapped
    IDirect3DVertexDeclaration9* decl;
    int streamelements[16];
    int streamusage[16];
    int streamusageindex[16];
};

struct ShaderUniformParameters
{
    unsigned int* samplerHW;
    unsigned int* uniformHW;
};

struct ShaderVaryingParameters
{
    unsigned int* streamHW;
    unsigned int* uvsetsHW;
};

struct ShaderTreeSolution
{
    unsigned int uvmask;
    unsigned int sourceHash;
    ShaderProgram* vertexprog;
    ShaderProgram* pixelprog;
    ShaderUniformParameters vertex;
    ShaderUniformParameters pixel;
    ShaderVaryingParameters varying;
    ShaderTreeDefinition* definition;
};


struct ShaderTreeDefinition
{
    ShaderUniform vertex;
    ShaderUniform pixel;
    ShaderVarying input;
    ShaderDefineSet* dset;
    ShaderTreeSolution* solution;
    unsigned int materialflags;
};

struct ShaderTreeDefinitionGroup
{
    ShaderTreeDefinitionGroup* next;
    $21D7E06F25E7019387B855BD7632CB52 ___u1;
    ShaderTreeDefinition* group[];
};

struct $A9F7334127035CE6D120363DE06063F7
{
    unsigned __int32 : 30;
    unsigned __int32 rectangle : 1;
    unsigned __int32 clamptex : 1;
};

union $2770EB137B66BB8F764F63C7788D3B8C
{
    unsigned int all;
    $A9F7334127035CE6D120363DE06063F7 __s1;
};

struct ShaderTarget
{
    const char* name;
    unsigned int format;
    unsigned int width;
    unsigned int height;
    unsigned int depth;
    unsigned int pbh;
    ShaderVariable* var;
    $2770EB137B66BB8F764F63C7788D3B8C flags;
};

enum ShaderCallType : __int32
{
    SHADER_PASS_START = 0x0,
    SHADER_PASS_END = 0x1,
};

enum gfxPassType : __int32
{
    GFXPASS_INVALID = 0x0,
    GFXPASS_PREPASS = 0x1,
    GFXPASS_AMBIENT = 0x2,
    GFXPASS_SHADOW = 0x4,
    GFXPASS_LIGHT = 0x8,
    GFXPASS_ALPHA_AMBIENT = 0x10,
    GFXPASS_ALPHA_LIT = 0x20,
    GFXPASS_VOLUME = 0x40,
    GFXPASS_DEPTH = 0x80,
    GFXPASS_PROJECT = 0x100,
    GFXPASS_INCANDESCENCE = 0x200,
    GFXPASS_REFRACTION_MASK = 0x400,
    GFXPASS_REFRACTION = 0x800,
    GFXPASS_DEBUG = 0x1000,
    GFXPASS_WATER_REFL = 0x2000,
    GFXPASS_WATER_MASK = 0x4000,
    GFXPASS_WATER = 0x8000,
    GFXPASS_WATER2_MASK = 0x10000,
    GFXPASS_WATER2 = 0x20000,
    GFXPASS_SKIN_DEPTH = 0x40000,
    GFXPASS_OCCLUSION = 0x80000,
    GFXPASS_FOG = 0x100000,
    GFXPASS_AMBLIT = 0x200000,
    GFXPASS_DISTFOG = 0x400000,
    GFXPASS_CHECKVIS = 0x800000,
    GFXPASS_CHECKVISPRE = 0x1000000,
    GFXPASS_HDRBEGIN = 0x2000000,
    GFXPASS_HDREND = 0x4000000,
    GFXPASS_UNFILTERED_ALPHA = 0x8000000,
    GFXPASS_LODBLEND = 0x10000000,
};

struct gfxPassParams_s
{
    gfxPassType type;
    unsigned int light;
    viewdef_s* view;
    mtx_u* viewTexture;
    mtx_u* invrottrans;
    int numLights;
    unsigned int blendMaskOverride;
    unsigned __int16* buffer;
    int bufferMax;
    unsigned __int8 forceAlpha;
    frustumdef_s* spotFrustum;
    mtx_u* skinDMapMatrix;
    int oldDepthWrite;
};

struct __declspec(align(16)) ShaderParams_s
{
    mtx_u obMatrix;
    vec3_u* obViewPos;
    vec3_u* obLightPos;
    vec3_u* viewPos;
    vec3_u* viewDir;
    vec3_u* lightPos;
    int shadowed;
    float* lcol;
    float* latten;
    float* lightSize;
    float* projMat;
    float* projMat2;
    vec4_u* constantColour;
    void* callbackData;
};

struct ShaderPassInfo
{
    const char* name;
    char* nameUNI;
    int(__cdecl* setupFN)(ShaderCallType, ShaderPassInfo*);
    unsigned int flags;
    ShaderTarget* target;
    gfxPassType type;
    unsigned int index;
    gfxPassParams_s* passparams;
    ShaderParams_s* shaderparams;
    viewdef_s* view;
    struct opaqueProp** props;
    int numprops;
    unsigned int enabled;
    ShaderPassInfo* next;
    ShaderPassInfo* child;
    ShaderPassInfo* parent;
};

struct ShaderTreeIteration
{
    unsigned int nameLen;
    const char* name;
    ShaderTreeDefinitionGroup* definitions;
    ShaderTreeIteration* next;
    ShaderTarget* target;
    int(__cdecl* setupFN)(ShaderCallType, ShaderPassInfo*);
    unsigned int index;
    unsigned int currentState;
    unsigned int numStates;
};

struct opaqueProp
{
};