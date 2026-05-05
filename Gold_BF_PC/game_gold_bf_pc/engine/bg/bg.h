#pragma once
#include "Util/unorgtypes.h"
struct propdef_s;
struct bginstdef_s;

struct floordef_s
{
	vec3_u norm;
	unsigned int pmh;
	unsigned int numVerts;
	vec3_u* vert;
};

struct roomdef_s
{
	void* ob;
	aabbdef_u bb;
	int numportals;
	int* portals;
	vec3_u pos;
	vec3_u rot;
	int physicsID;
	unsigned int numFloors;
	floordef_s* floor;
};

struct portaldef_s
{
	int proom;
	int nroom;
	vec3_u norm;
	aabbdef_u bb;
	int numpoints;
	int inMaterial;
	int gameMaterial;
	vec3_u* points;
};

struct __declspec(align(4)) fluidvolumedef_s
{
	aabbdef_u aabb;
	vec4_u plane[2];
	unsigned __int8 numvertexes;
	vec2_u* vertexes;
	char name[16];
	char type[16];
	unsigned __int8 id;
};

struct lightvolgroup_s
{
	int num;
	lightvol_s* vols[1];
};

struct BgFileData
{
	roomdef_s* rooms;
	int numrooms;
	portaldef_s* portals;
	unsigned int numportals;
	fluidvolumedef_s* fluidvols;
	unsigned int numfluidvols;
	lightvolgroup_s* lightvolgrp;
	matinfodef_s* materials;
	unsigned int nummaterials;
	char* texturesrcnames;
	unsigned int* object_names;
	char* object_strings;
	unsigned int* objectgroup_names;
	char* objectgroup_strings;
};

struct kdTree
{
	unsigned int* nodes;
	unsigned __int16* data;
	float bbMin[3];
	float bbMax[3];
};

struct bgdef_s
{
	BgFileData* header;
	unsigned int hdrMem;
	unsigned int blkMem;
	void* _buffer;
	void* _blkBuffer;
	unsigned int refCount;
	unsigned int ftHandle;
	unsigned __int16 numrooms;
	unsigned __int16 numRoomGroups;
	unsigned __int16 numportals;
	roomdef_s* rooms;
	int* roomGroups;
	portaldef_s* portals;
	kdTree roomTree;
	obdef_s** obs;
	unsigned __int8 streamState;
	unsigned __int8 isPersistent;
	unsigned __int8 hasPreloadedMaterials;
	unsigned __int8 pad;
	lightvolgroup_s* lightvolgrp;
	stringTable* roomGroupStringTable;
	stringTableElement* roomGroupSTPtrs[4];
	stringTableElement* roomGroupSTElements;
};

struct guPortalDef_s
{
	vec3_u midPoint;
	const char* name;
	unsigned int pBg;
	unsigned int nBg;
	int pBgRoom;
	int nBgRoom;
	portaldef_s* portal;
};

struct __declspec(align(16)) bginstTransformData_s
{
	mtx_u m;
	vec3_u vel;
	vec3_u angVel;
};

union $E10814959B10658D92B02F26E6E42745
{
	propdef_s* prop;
	bginstdef_s* bginst;
};

enum SCENE_OBJ : __int32
{
	SCENE_OBJ_OB = 0x0,
	SCENE_OBJ_BG = 0x1,
};

struct SceneObject_s
{
	slinkdef_s link;
	$E10814959B10658D92B02F26E6E42745 ___u1;
	SCENE_OBJ type;
};

struct __declspec(align(8)) propdef_s
{
	slinkdef_s linkall;
	struct opaqueObInst* insth;
	void(__cdecl* render)(struct opaqueProp*, ShaderParams_s*, gfxPassParams_s*);
	void(__cdecl* expressionVars)(struct opaqueProp*);
	unsigned int flags;
	unsigned int type;
	void* user;
	void* node; //proptree_s
	mtx_u matrix;
	aabbdef_u localBounds;
	unsigned __int8 drawMe;
	SceneObject_s* sceneobj;
	float gameTimeSinceLastDrawn;
	BgRoomGroup_s bgRoomGroup;
};

struct bginstdef_s
{
	unsigned int bgh;
	bginstTransformData_s* transforms;
	aabbdef_u aabb;
	struct opaqueProp** props;
	SceneObject_s* sceneobj;
	unsigned __int8* visbits;
	unsigned __int8* portalsDisabled;
	int* overidestate;
	void* user;
	slinkdef_s linkall;
	int loadStatus;
	float cmlVerticalOffset;
	char name[128];
};

struct bgInstListElement_s
{
	bginstdef_s* bginst;
	unsigned int bgh;
};

struct __declspec(align(4)) guPortalArray_s
{
	guPortalDef_s portals[37];
	unsigned int numPortals;
	unsigned __int8 isValid;
};

struct bgClipData
{
	__int16 scrmin[2];
	__int16 scrmax[2];
};

struct bgStreamEntry_s
{
	int fileSize;
	int fileHandle;
	unsigned __int8* buffer;
	unsigned int hmem;
	unsigned int bgh;
	__int16 setupNum;
	unsigned __int8 type;
	unsigned __int8 retry;
};

struct worldRoomDef
{
	int visible;
	bgClipData clipRect;
	frustumdef_s clipFrustum;
};

struct bgInstCalcData
{
	bginstdef_s* bginst;
	int startroom;
	unsigned __int8 roomFlags[500];
	bgClipData roomClip[500];
	unsigned __int8 portalFlags[200];
	bgClipData portalClip[200];
};

struct sdkMeshData_s
{
	void* sdkMesh;
	unsigned int refcount;
	int objectIdx;
	int partIdx;
	unsigned int flags;
	__declspec(align(16)) mtx_u transform;
};

const struct physicsResource_s
{
	unsigned int fth;
	unsigned int numMeshes;
	sdkMeshData_s* meshData;
};

struct $85B2A06066A5FFCFFD044BEC3290C8BB
{
	int numbgs;
	bgInstCalcData data[8];
};

void bgReset();