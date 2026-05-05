#include "engineinit.h"
#include <windows.h>

rendererdebug_s rdebugflags;
EngineInitData initData;
unsigned __int8 g_enableHotReload = 0u;
unsigned __int8 s_soundEnable = 1u;
unsigned __int8 s_soundJIT = 0u;
float s_shadowQuality = 0.5;
unsigned __int8 sForceLDR = 0u;
unsigned __int8 sHdrEnabledNext = 0u;
unsigned __int8 s_hdrFirstFrame[4] = { 0u, 0u, 0u, 0u };
unsigned __int8 g_disableMipmaps = 0u;
char* g_unitName = nullptr;
unsigned __int8 cacheSyncEnabled = 1u;
unsigned __int8 cacheEnabled = 0u;

float maxPostAttackAngleDot = -0.2;
float c_PhantomMovementExpansionRatio = 3.0;
float radius = 0.15000001;
float maxLimit = 0.69999999;
float s_depthCutoffDist = 300.0;
float s_mpRate = 32.0;
unsigned int  timerRenderFrameNum = 0;

unsigned int timerGameFrameNum = 0u;
double timerGameTime = 0.0;
double timerTargetGameTime = 0.0;
float timerLastFrameDuration = 0.0;
float timerLastGameFrameDuration = 0.0;
double timerFrameStartTime = 0.0;

settings_res* g_settings = NULL;
//CDictionary* g_settings = NULL;

f_constants_res* g_constants = NULL;
constants_ai_res* g_constants_ai = NULL;
constants_health_and_damage_res* g_constants_healt_hand_damage = NULL;
bf_constants_ai_res* g_bf_constants_ai = NULL;
bf_constants_res* g_bf_constants = NULL;

const char* s_hwPath = "d:\\";

//Render flags setup
void rendererDebugInitialise()
{
    memset(&rdebugflags, 0, sizeof(rendererdebug_s));
    rdebugflags.texstreamhi = 0;
    rdebugflags.screenshot_first = 1;
    rdebugflags.dbgMMChangeSpd = 1.0;
    rdebugflags.drawmode = 0;
    rdebugflags.lmapOfs = 0.1;
    rdebugflags.disableFoliageGfx = 0;
    rdebugflags.lmapDist = 0.1;
    rdebugflags.drawdecals = 1;
    rdebugflags.lmapsat = 1.0;
    rdebugflags.drawwater = 1;
    rdebugflags.grassdrawdist = 20.0;
    rdebugflags.batchmode = 0;
    rdebugflags.rimlightWidth = 0.44999999;
    rdebugflags.dbgmip = 0;
    rdebugflags.rimlightFade = 0.30000001;
    rdebugflags.dbgDiffStreamedLevel = 0;
    rdebugflags.rimlightValue = 1.0;
    rdebugflags.dbgLMapStreamedLevel = 0;
    rdebugflags.uselmap = 1;
    rdebugflags.usedif = 1;
    rdebugflags.usenrm = 1;
    rdebugflags.usespec = 1;
    rdebugflags.minTexStreamLoadLevel = 0;
    rdebugflags.noenginetick = 0;
    rdebugflags.dbgMinMipPack = 0;
    rdebugflags.dbgMaxMipPack = 3;
    rdebugflags.dbgMipmapBase = 4;
    rdebugflags.dbgStreamRate = 0;
    rdebugflags.dbgStreamDelay = 0;
    rdebugflags.prepassenabled = 0;
    rdebugflags.forceenvmap = 0;
    rdebugflags.wireframe = 0;
    rdebugflags.bindpose = 0;
    rdebugflags.locklevels = 0;
    rdebugflags.killterrain = 0;
    rdebugflags.killgrass = 1;
    rdebugflags.bpatch = 0;
    rdebugflags.stitchonly = 0;
    rdebugflags.drawshadows = 1;
    rdebugflags.forceShadowsOff = 0;
    rdebugflags.drawbackshadows = 1;
    rdebugflags.enablelights = 1;
    rdebugflags.enablerimlight = 1;
    rdebugflags.shaderUseNdotL = 1;
    rdebugflags.drawlights = 0;
    rdebugflags.drawbglights = 0;
    rdebugflags.drawportals = 0;
    rdebugflags.drawwrcliprect = 0;
    rdebugflags.drawocc = 0;
    rdebugflags.drawfloors = 0;
    rdebugflags.drawfloorhits = 0;
    rdebugflags.drawbbs = 0;
    rdebugflags.drawellips = 0;
    rdebugflags.ellipsDetail = 10;
    rdebugflags.drawspheres = 0;
    rdebugflags.sphereDetail = 10;
    rdebugflags.drawPart = -1;
    rdebugflags.drawBBText = 0;
    rdebugflags.drawroombbs = 0;
    rdebugflags.drawbgbs = 0;
    rdebugflags.drawrbs = 0;
    rdebugflags.drawsky = 1;
    rdebugflags.linkFlarePosWithSun = 1;
    rdebugflags.disableSunDecal = 0;
    rdebugflags.disableSpacePosUpdate = 0;
    rdebugflags.drawclouds = 1;
    rdebugflags.drawterrain = 1;
    rdebugflags.useterraincallback = 1;
    rdebugflags.usefloors = 1;
    rdebugflags.enablePropDrawing = 1;
    rdebugflags.enableBgDrawing = 1;
    rdebugflags.showTerrainLevels = 0;
    rdebugflags.updateTerrain = 1;
    rdebugflags.updateTerrainEarlyUpdate = 1;
    rdebugflags.updateTerrainMultithreaded = 1;
    rdebugflags.updateTerrainAsynchronously = 0;
    rdebugflags.useTerrainStreamedMegatex = 1;
    rdebugflags.terrainDiffuseAmbient = 0.25;
    rdebugflags.updateMegatex = 1;
    rdebugflags.terrainSunWeight = 0.0;
    rdebugflags.terrainDebugShadow = 0;
    rdebugflags.propContribCullSize = 5.0;
    rdebugflags.backgroundCastOntoTerrain = 1;
    rdebugflags.propCullDist = 1000.0;
    rdebugflags.terrainShadowExpensiveFilter = 1;
    rdebugflags.brightness = 1.0;
    rdebugflags.terrainUseLightmapNear = 1;
    rdebugflags.gamma = 1.0;
    rdebugflags.terrainUseLightmapFilter = 1;
    rdebugflags.grassParam1 = 1.4;
    rdebugflags.alphaasopa = 0;
    rdebugflags.grassParam2 = 1.0;
    rdebugflags.particleDraw = 1;
    rdebugflags.grassParam3 = 0.0;
    rdebugflags.particleDrawCML = 1;
    rdebugflags.grassParam4 = 0.5;
    rdebugflags.particleDrawBounds = 0;
    rdebugflags.ati = 1.5;
    rdebugflags.particleDrawGeoms = 1;
    rdebugflags.gunZRange = 0.0099999998;
    rdebugflags.particleDrawInfo = 0;
    rdebugflags.PCFScale = 1.0;
    rdebugflags.particleDrawOverdrawShader = 0;
    rdebugflags.shadowAtten = c_PhantomMovementExpansionRatio;
    rdebugflags.particleDrawSimpleShader = 0;
    rdebugflags.shadowZOffset = 0.0;
    rdebugflags.particleDrawSprites = 1;
    rdebugflags.blobAngle = 1.0;
    rdebugflags.particleTickCollisions = 1;
    rdebugflags.blobRadius = 1.5;
    rdebugflags.particleSorting = 0;
    rdebugflags.envMapIncan = 0.30000001;
    rdebugflags.particleResolution = 0;
    rdebugflags.envMapMinClamp = radius;
    rdebugflags.particleDrawSystemNames = 0;
    rdebugflags.envMapScale = 1.0;
    rdebugflags.drawforces = 0;
    rdebugflags.CMLSunClamp = 0.30000001;
    rdebugflags.drawweathers = 0;
    rdebugflags.lightBlendTime = radius;
    rdebugflags.drawproptree = 0;
    rdebugflags.drawcollisionmesh = 0;
    rdebugflags.drawlinetestmesh = 1;
    rdebugflags.drawcollisionmesh_filled = 0;
    rdebugflags.drawcollisionmesh_colsFromNormal = 0;
    rdebugflags.drawcollisionmesh_readFromDbgMgr = 1;
    rdebugflags.drawcollisionmesh_player_shoot_through = 0;
    rdebugflags.drawcollisionmesh_ai_shoot_through = 0;
    rdebugflags.drawcollisionmesh_ai_see_through = 0;
    rdebugflags.shadowculling = 1;
    rdebugflags.combineambientlit = 0;
    rdebugflags.viewshadowbuffer = 0;
    rdebugflags.drawElements = 1;
    rdebugflags.renderBatchCulling = 1;
    rdebugflags.frustumCullBgSurfaces = 1;
    rdebugflags.homTestPoly2DEnabled = 1;
    rdebugflags.bginstAABBFrustumClipping = 1;
    rdebugflags.roomAABBWorldRoomFrustumClipping = 1;
    rdebugflags.bgUsePortalsForVisRooms = 1;
    rdebugflags.useOBBFrustumClip = 1;
    rdebugflags.usePropContribCulling = 1;
    rdebugflags.propOcclusionGFXEnabled = 0;
    rdebugflags.propDistCullingEnabled = 0;
    rdebugflags.shadercache = 1;
    rdebugflags.cpuSkinOptimisations = 1;
    rdebugflags.enableSkinning = 1;
    rdebugflags.skinvbo = 1;
    rdebugflags.varyingcache = 1;
    rdebugflags.uniformcache = 1;
    rdebugflags.lightLineDraw = 0;
    rdebugflags.compressHDR = 1;
    rdebugflags.grassParam5 = 3;
    rdebugflags.seperateGunZRange = 1;
    rdebugflags.multisample = 0;
    rdebugflags.incanSetting = 0;
    rdebugflags.displayBadCML = 0;
    rdebugflags.detachCML = 0;
    rdebugflags.CMLVerticalHack = 0.0;
    rdebugflags.sortLayersFirst = 1;
    rdebugflags.lightingWrapValue = 0.0;
    rdebugflags.frameratelock = 3;
    rdebugflags.lightLOD = 1.0;
    rdebugflags.debugBackgroundShadows = 0;
    rdebugflags.lightHackScale = maxLimit;
    rdebugflags.usefrontfaces = 1;
    rdebugflags.lightHackDistance = 10.0;
    rdebugflags.separatebgsb = 0;
    rdebugflags.lightHackVertOffset = 2.0;
    rdebugflags.terrainShadowProject = 1;
    rdebugflags.blurSkinLMapFactor = 1.0;
    rdebugflags.fastproject = 1;
    rdebugflags.shadowBufOffsetScale = 1.1;
    rdebugflags.omptest = 0;
    rdebugflags.shadowBufOffsetTranslate = 4.0;
    rdebugflags.ignoredAssertPrintfs = 1;
    rdebugflags.shadowCloseDistClamp = 0.5;
    rdebugflags.printfs = 1;
    rdebugflags.texelsPerMeter = 45.0;
    rdebugflags.navDrawDist = -5;
    rdebugflags.maxInGameTexRes = 1024.0;
    rdebugflags.drawnormals = 0;
    rdebugflags.lodMergeClipdist = s_depthCutoffDist;
    rdebugflags.drawldlc = 0;
    rdebugflags.detailPartsCullingOverrideDist = 2000.0;
    rdebugflags.statsDisplay = 0;
    rdebugflags.overrideSkyFOV = 0.0;
    rdebugflags.statsObDisplay = 0;
    rdebugflags.scaleOptimalFilter = 0.40000001;
    rdebugflags.statsTexDisplay = 0;
    rdebugflags.statsRoomDisplay = 0;
    rdebugflags.fasterShadowCast = 1;
    rdebugflags.useSphericalH = (initData.Viewer == 0);
    rdebugflags.useSphericalHBG = 1;
    rdebugflags.displaySkinLMap = 0;
    rdebugflags.lightStats = 0;
    rdebugflags.skinbfculltest = 0;
    rdebugflags.shadowGroup = 0;
    rdebugflags.shadowDebugDraw = 0;
    rdebugflags.spotlightCull = 1;
    rdebugflags.enableDetailGeom = 1;
    rdebugflags.fadePortalsEnabled = 1;
    rdebugflags.debugTex = 0;
    rdebugflags.debugTexIdx = 0;
    rdebugflags.envMapEnable = 1;
    rdebugflags.dynamicEnvMapEnable = 0;
    rdebugflags.envMapDebugGFX = 0;
    rdebugflags.envMapPriority = 0;
    rdebugflags.envMapCamFOV = 90;
    rdebugflags.envMapLighting = 1;
    rdebugflags.lodObEnable = 1;
    rdebugflags.lodObShadowsEnable = 0;
    rdebugflags.lodObReduction = 0;
    rdebugflags.lodObShadowReduction = 1;
    rdebugflags.lodObOverrideEnable = 0;
    rdebugflags.lodObOverrideLevel = 0;
    rdebugflags.lodObOverrideDist = 5;
    rdebugflags.lodPartEnable = 1;
    rdebugflags.lodPartOverrideEnable = 0;
    rdebugflags.lodPartOverrideLevel = 0;
    rdebugflags.lodPartMergeEnable = 1;
    rdebugflags.lodPartEllipsoidDist = 0;
    rdebugflags.lodBlendEnable = 1;
    rdebugflags.lodBlendSpeed = 16;
    rdebugflags.detailPartsCullingEnable = 1;
    rdebugflags.detailPartsCullingOverrideEnable = 0;
    rdebugflags.detailPartsCullingOverrideAlwaysCull = 0;
    rdebugflags.screenshot_width = 1;
    rdebugflags.screenshot_height = 1;
    rdebugflags.screenshot_count = 0;
    rdebugflags.sortPasses = 1;
    rdebugflags.sdHideChanges = 0;
    rdebugflags.simpleHdr = 0;
    rdebugflags.grassGfxBefore = 0;
    rdebugflags.perceptionTest = 0;
    rdebugflags.enableTextures = 1;
    rdebugflags.enableHighDef = 1;
    rdebugflags.shadowProjectWorkaround = 0;
    rdebugflags.perf = 0;
    rdebugflags.e3ShadowCullDist = 25.0;
    rdebugflags.perfhdrdisable = 0;
    rdebugflags.testScaleTerrain = 1.0;
    rdebugflags.drawPropsBeforeBg = 1;
    rdebugflags.foliageWindDirection = 0.0;
    rdebugflags.memPrintAll = 0;
    rdebugflags.foliageDrawDistMin = 20.0;
    rdebugflags.texstream = 1;
    rdebugflags.foliageDrawDistMax = s_mpRate;
    rdebugflags.loadingMessage = 0;
    rdebugflags.foliageDrawDistFadeLen = 4.0;
    rdebugflags.terrainDebugSlopes = 0;
    rdebugflags.foliageDensityScale = 1.0;
    rdebugflags.foliageDisplayStats = 0;
    rdebugflags.dkshieldBias = 1.0;
    rdebugflags.dkshieldScale = maxPostAttackAngleDot;
    rdebugflags.enableAlternativeTerrain = 0;
    rdebugflags.dkshieldPower = c_PhantomMovementExpansionRatio;
    rdebugflags.terrainHeightfieldLevel = 0;
    rdebugflags.dkshieldFeedbackStrength = 0.60000002;
    rdebugflags.foliageDebugType = -1;
    rdebugflags.heightfieldTau = 6.0;
    rdebugflags.heightfieldBorderBlend = 0;
    rdebugflags.heightfieldNearVal = 1.0;
    rdebugflags.heightfieldTopVal = 1.0;
    rdebugflags.healthEffectDuration = 0.40000001;
    rdebugflags.healthEffectIntensity = 0.40000001;
}

//Super simple c func
int engineHandleArguments(int argc, char** argv) 
{
    rendererDebugInitialise();
    int v4 = 1;

    if (argc > 1) {
        char** v5 = argv + 1;
        char* v6 = reinterpret_cast<char*>(argv + 2);

        while (v4 < argc) {
            char* v7 = *v5;

            if (**v5 == '+') {
                char* v8 = v7 + 1;

                if (!_stricmp(v8, "reload")) {
                    rdebugflags.enableHotFileReloading = 1;
                }
                else if (!_stricmp(v8, "sunnyBg")) {
                    rdebugflags.dynamicSunlight = 1;
                }
                else if (!_stricmp(v8, "noReload")) {
                    rdebugflags.enableHotFileReloading = 0;
                }
                else if (!_stricmp(v8, "texStream")) {
                    rdebugflags.texstream = 1;
                    g_enableHotReload = 0;
                }
                else if (!_stricmp(v8, "noTexStream")) {
                    rdebugflags.texstream = 0;
                }
                else if (!_stricmp(v8, "noSound")) {
                    s_soundEnable = 0;
                }
                else if (!_stricmp(v8, "jitSound")) {
                    s_soundJIT = 1;
                }
                else if (!_stricmp(v8, "noJitSound")) {
                    s_soundJIT = 0;
                }
                else if (!_stricmp(v8, "noShadows")) {
                    rdebugflags.drawshadows = 0;
                    rdebugflags.forceShadowsOff = 1;
                }
                else if (!_strnicmp(v8, "shadowQuality", 15)) {
                    s_shadowQuality = static_cast<float>(atof(v8 + 15));
                }
                else if (!_stricmp(v8, "hdr")) {
                    if (!sForceLDR) {
                        if (!sHdrEnabledNext) {
                            memset(s_hdrFirstFrame, 1, sizeof(s_hdrFirstFrame));
                        }
                        sHdrEnabledNext = 1;
                    }
                }
                else if (!_stricmp(v8, "noHdr")) {
                    sForceLDR = 1;
                }
                else if (!_stricmp(v8, "noMipmaps")) {
                    g_disableMipmaps = 1;
                }
                else if (!strncmp(v8, "unitName", 8)) {
                    g_unitName = strstr(v8, "=") + 1;
                }
                else if (!_stricmp(v8, "noTex")) {
                    rdebugflags.enableTextures = 0;
                }
                else if (!_stricmp(v8, "highDef")) {
                    rdebugflags.enableHighDef = 1;
                }
                else if (!_stricmp(v8, "jit")) {
                    rdebugflags.shaderJitCompile = 1;
                }
                else if (!_stricmp(v8, "quit")) {
                    rdebugflags.forceQuit = 1;
                }
                else if (!_stricmp(v8, "texReload")) {
                    g_enableHotReload = 1;
                    rdebugflags.texstream = 0;
                }
                else if (!_stricmp(v8, "noDevcacheSync")) {
                    cacheSyncEnabled = 0;
                }
                else if (!strncmp(v8, "devcache", 8)) {
                    cacheEnabled = 1;
                }
                else if (!strncmp(v8, "overrideRootFo", 14)) {
                    // This seems to indicate a flag without specific behavior
                }
                else {
                    ++v4;
                    ++v5;
                    v6 += sizeof(char*);
                    continue;
                }

                if (v4 < argc) {
                    memmove(v5, v6, sizeof(char*) * (argc - v4));
                }
                --argc;
                --v4;
                --v5;
                v6 -= sizeof(char*);
            }

            ++v4;
            ++v5;
            v6 += sizeof(char*);
        }
    }
    return argc;
}