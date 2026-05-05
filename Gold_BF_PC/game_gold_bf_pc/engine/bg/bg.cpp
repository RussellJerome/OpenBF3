#include "bg.h"
#include "engine/mem.h"
#include "engine/platform_pc/hw.h"

int s_renderallforframe = 0;
bgdef_s bgarray[32];
guPortalArray_s s_guPortals;
bgInstListElement_s g_bginstTable[24];
slinklistdef_s bgInstListAll;
worldRoomDef s_worldRoom;
bgStreamEntry_s s_bgInFlight[2];
$85B2A06066A5FFCFFD044BEC3290C8BB bgCalcData;
unsigned int s_numInFlight = 0u;

void bgFreeRoomTree(bgdef_s* bg)
{
    if (!bg->roomTree.nodes)
        return;

    memFreeFlags((char*)bg->roomTree.nodes, 1);
    memFreeFlags((char*)bg->roomTree.data, 1);
    bg->roomTree.nodes = nullptr;
    bg->roomTree.data = nullptr;
}

void bgUnloadHeightMap(unsigned int bgh)
{
    bgdef_s* v5 = &bgarray[bgh];

    //TODO
    //pmeshFree((pmesh_s*)v5->_buffer);
    memFreeFlags((char*)v5->header, 1);
    memFreeFlags((char*)v5->roomGroupStringTable, 1);
    memFreeFlags((char*)v5->roomGroupSTElements, 1);
    memFreeFlags((char*)v5->roomGroups, 1);

    if (v5->roomTree.nodes)
    {
        memFreeFlags((char*)v5->roomTree.nodes, 1);
        memFreeFlags((char*)v5->roomTree.data, 1);
        v5->roomTree.nodes = nullptr;
        v5->roomTree.data = nullptr;
    }

    fileTableRemove(v5->ftHandle);
    v5->ftHandle = -1;

    memFreeFlags((char*)v5->lightvolgrp, 1);

    //if (s_terrainNavMesh)
    //{
    //    s_terrainNavMesh->~CTerrainNavMesh();
    //    s_terrainNavMesh = nullptr;
    //}
}

void bgUnload(unsigned int bgh)
{
    if (bgh == (unsigned int)-1)
        return;

    bgdef_s* v5 = &bgarray[bgh];
    unsigned int refCount = v5->refCount;
    BgFileData* header = v5->header;

    if (!refCount)
        return;

    unsigned int v8 = refCount - 1;
    v5->refCount = v8;
    obdef_s** obs = v5->obs;

    if (obs && *obs && ((*obs)->flags & 0x20) != 0)
    {
        if (v8)
            return;
        bgUnloadHeightMap(bgh);
        //goto cleanup;
    }

    if (!v5->_buffer || v8)
        return;

    // Cancel any in-flight streaming for this bgh
    unsigned __int8** p_buffer = &s_bgInFlight[0].buffer;
    while ((int)p_buffer < (int)&bgCalcData.data[0].startroom)
    {
        if (p_buffer[2] == (unsigned __int8*)bgh)
        {
            fileStreamingClose((unsigned __int16)(uintptr_t) * (p_buffer - 1));
            memFreeFlags((char*)*p_buffer, 1);
            *p_buffer = nullptr;
            p_buffer[2] = (unsigned __int8*)-1;
            *(p_buffer - 1) = (unsigned __int8*)-1;
            *((WORD*)p_buffer + 6) = 0;
            *((BYTE*)p_buffer + 15) = 0;
            --s_numInFlight;
        }
        p_buffer += 6;
    }

    //bgFreeBlock(bgh);

    //if (s_guPortals.numPortals)
        //bgRemoveBgFromGuPortalList(bgh);

    // Remove fluid volumes
    //for (unsigned int i = 0; i < header->numfluidvols; i++)
        //fluidVolumeRemove(&header->fluidvols[i]);

    // Remove physics resources
    char filename[0x150];
    if (fileTableGetFilename(filename, v5->ftHandle))
    {
        if (!s_guPortals.isValid && v5->ftHandle != (unsigned int)-1)
        {
            //physicsResource_s* phys = physicsFileFindPhysicsResource(v5->ftHandle);
            //if (phys)
            //{
            //    physicsFileUnLoadAllMeshes(phys);
            //    physicsFileRemovePhysicsResource(phys);
            //}
        }
    }

    fileTableRemove(v5->ftHandle);
    v5->ftHandle = -1;
    v5->streamState = 0;

    memFreeFlags((char*)v5->roomGroupStringTable, 1);
    memFreeFlags((char*)v5->roomGroupSTElements, 1);
    memFreeFlags((char*)v5->roomGroups, 1);
    bgFreeRoomTree(&bgarray[bgh]);
    memFreeFlags((char*)v5->obs, 1);
    memFreeFlags((char*)v5->_buffer, 1);
    v5->_buffer = nullptr;

cleanup:
    memset(&bgarray[bgh], 0, 0x74);
}

void bgReset()
{
    s_renderallforframe = 0;

    // Unload all 32 background slots
    for (int i = 0; i < 32; i++)
        bgUnload(i);

    // Clear background array
    memset(bgarray, 0, 0xE80);

    // Reset portal list
    float* v2 = &s_guPortals.portals[0].midPoint.v[1];
    while ((int)v2 < (int)&s_guPortals.isValid)
    {
        v2[-1] = 0.0f;
        v2[0] = 0.0f;
        v2[1] = 0.0f;
        v2[2] = 0.0f;
        v2[3] = -1;   // NAN in original = -1 as int (0xFFFFFFFF)
        v2[4] = -1;
        v2[5] = -1;
        v2[6] = -1;
        v2[7] = 0.0f;
        v2 += 9;
    }
    s_guPortals.numPortals = 0;
    s_guPortals.isValid = 0;

    // Reset bginst table
    unsigned int* p_bgh = &g_bginstTable[0].bgh;
    while ((int)p_bgh < (int)&bgInstListAll.offset)
    {
        *(p_bgh - 1) = 0;
        *p_bgh = -1;
        p_bgh += 2;
    }

    // Reset in-flight background loads
    int* p_fileHandle = &s_bgInFlight[0].fileHandle;
    while ((int)p_fileHandle < (int)bgCalcData.data)
    {
        *(p_fileHandle - 1) = 0;
        *p_fileHandle = -1;
        p_fileHandle[3] = -1;
        p_fileHandle[1] = 0;
        *((WORD*)p_fileHandle + 8) = 0;
        p_fileHandle += 6;
    }

    // Reset world room
    s_worldRoom.visible = 0;

    // Reset bgInstListAll linked list
    bgInstListAll.offset = -56;
    bgInstListAll.head = (slinkdef_s*)&bgInstListAll;

    // Reset world room clip rect to max extents
    s_worldRoom.clipRect.scrmin[0] = 0x7FFF;
    s_worldRoom.clipRect.scrmax[0] = -0x8000;
    s_worldRoom.clipRect.scrmin[1] = 0x7FFF;
    s_worldRoom.clipRect.scrmax[1] = -0x8000;
}