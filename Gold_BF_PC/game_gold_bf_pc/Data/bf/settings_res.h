#pragma once
//Dumped from Corra Build
//TODO Dump it from the og r7 since I fucked it up the first time

class settings_res
{
public:
    settings_res() = default;

    const char* FileName = "data/bf/settings.res";
    int Count = 43;

    const char* physicssdk = "havok";
    const char* goldCheck = "false";
    const char* stacktraceprintfs = "false";
    const char* disableprintfs = "false";
    const char* disableasserts = "false";
    const char* disabledAssertsStillPrint = "false";
    const char* memdbglevel = "low";
    const char* memlogallocs = "false";
    const char* no_sound = "false";
    const char* gtkEnabled = "true";
    const char* forceVsync = "true";
    const char* displayBuildNum = "false";
    const char* statusBarDisplaysGU = "false";
    const char* statusBarDisplaysBG = "true";
    const char* showWatermarkWarning = "true";
    const char* default_language = "english";
    const char* default_languagecode = "eng";

    struct
    {
        const char* COMMON = "../../common/buildfiles/resfiles";
        const char* VMDOC = "data/common/debug/vm";
    }Path;

    int conspiregeneratedotfile = 0;
    const char* conspiredotfilename = "plangraph.dot";
    int conspiredotlevels = 2;
    const char* conspireoutputdir = "planneroutput";
    int conspiredotlabelsshowmetrics = 1;
    int vmdoc_produceHTMLDocs = 0;
    const char* vmdoc_fileSkel = "$VMDOC/fileskel.htm";
    const char* vmdoc_funcSkel = "$VMDOC/funcskel.htm";
    const char* vmdoc_typeSkel = "$VMDOC/typeskel.htm";
    const char* vmdoc_enumSkel = "$VMDOC/enumskel.htm";
    const char* vmdoc_enumValueSkel = "$VMDOC/enumvalueskel.htm";
    const char* vmdoc_constSkel = "$VMDOC/constantvalueskel.htm";
    const char* vmdoc_outputFile = "vmdocs.htm";
    const char* autoCreatePlayerProfiles = "true";
    int lockFramerateTo = 0;
    const char* connectionType = "kConnectionType_lan";
    const char* assetsdir = "assets/bf";
    const char* enableLogoScene = "FALSE";
    const char* disableShaderUniformCache = "false";
    const char* instanceAllSetup = "true";
    const char* difficulty = "normal";
    int texstreamtickfreq = 12;
    const char* guiPCUsesPS3Pad = "true";
    const char* guiPCClickToHighlight = "false";
    const char* artDate = "00/00/00";
};