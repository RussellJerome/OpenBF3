#pragma once
#include "util/unorgtypes.h"

struct FontMap_s
{
    unsigned __int16 code;
    __int16 spare;
    unsigned __int16 fx;
    unsigned __int16 fy;
    unsigned __int16 fw;
    unsigned __int16 fh;
    __int16 xoff;
    __int16 yoff;
    __int16 xadv;
};

struct FontTransBlk_s
{
    unsigned __int16 Type;
    unsigned __int16 UniCodeBase;
    unsigned __int16 MapIndexBase;
    unsigned __int16 Range;
};


struct FontHeader_s
{
    unsigned __int16 magicnum;
    unsigned __int16 version;
    int numChannels;
    int bmWidth;
    int bmHeight;
    void* bitmap;
    int numMaps;
    int ymax;
    FontMap_s* fontMaps;
    unsigned int FontTexHandle;
    int numTrans;
    FontTransBlk_s* transtable;
    char* kerntable;
};

struct FontList_s
{
    FontHeader_s* font;
    float spaceWidth;
    unsigned int ftHandle;
    char assetName[64];
};

enum FontWindowBorderStyle : __int32
{
    FONT_WINDOW_BORDER_NONE = 0x0,
    FONT_WINDOW_BORDER_SOLID = 0x1,
    FONT_WINDOW_BORDER_OUTSET = 0x2,
    FONT_WINDOW_BORDER_INSET = 0x3,
    FONT_WINDOW_BORDER_DUMMY = 0xFFFFFFFF,
};

enum FontAlign : __int32
{
    FONT_ALIGN_LEFT = 0x0,
    FONT_ALIGN_CENTRE = 0x1,
    FONT_ALIGN_RIGHT = 0x2,
    FONT_ALIGN_FULL = 0x3,
    FONT_ALIGN_DUMMY = 0xFFFFFFFF,
};

enum FontCommand : __int32
{
    FontCommHandle = 0x0,
    FontCommPosition = 0x1,
    FontCommColourHex = 0x2,
    FontCommColour = 0x3,
    FontCommOutColour = 0x4,
    FontCommUnderLineColour = 0x5,
    FontCommAlpha = 0x6,
    FontCommOutAlpha = 0x7,
    FontCommUnderLineAlpha = 0x8,
    FontCommScale = 0x9,
    FontCommFixed = 0xA,
    FontCommAlign = 0xB,
    FontCommWindow = 0xC,
    FontCommMargin = 0xD,
    FontCommPrint = 0xE,
    FontCommNewLine = 0xF,
    FontCommSmLine = 0x10,
    FontCommSetFlag = 0x11,
    FontCommUnsetFlag = 0x12,
    FontCommDrawWin = 0x13,
    FontCommDefault = 0x14,
    FontCommSetTab = 0x15,
    FontCommBorderColour = 0x16,
    FontCommBorderColour2 = 0x17,
    FontCommBorderStyle = 0x18,
    FontCommBorderSize = 0x19,
    FontCommBorderPadding = 0x1A,
    FontCommSlider = 0x1B,
    FontCommDimensionCallback = 0x1C,
    FontCommCustomRenderCallback = 0x1D,
    FontCommPush = 0x1E,
    FontCommPop = 0x1F,
    FontDummy = 0x10000000,
};

struct __declspec(align(1)) $0F7D51B0C62DE9633A647472564C74BC
{
    unsigned int handle;
    FontHeader_s* header;
    FontColour_s colour;
    FontColour_s colourOutline;
    FontColour_s colourUnderline;
    float t;
    float s;
    float fixedw;
    unsigned int flag;
    float pw;
    float ph;
    float x;
    float y;
    float wx;
    float wy;
    float ww;
    float wh;
    windowdef_s* wdef;
    int cx;
    int cy;
    int cz;
    int cw;
    FontColour_s wbcolour;
    FontColour_s wbcolour2;
    FontWindowBorderStyle wbstyle;
    float wbsizex;
    float wbsizey;
    float wbpaddingx;
    float wbpaddingy;
    float mtop;
    float mbot;
    FontAlign align;
    unsigned __int8 fixFontScale;
};

struct __declspec(align(4)) FontStack_s
{
    unsigned int handle;
    FontHeader_s* header;
    FontColour_s colour;
    FontColour_s colourOutline;
    FontColour_s colourUnderline;
    float t;
    float s;
    float fixedw;
    unsigned int flag;
    float pw;
    float ph;
    float x;
    float y;
    float wx;
    float wy;
    float ww;
    float wh;
    windowdef_s* wdef;
    int cx;
    int cy;
    int cz;
    int cw;
    FontColour_s wbcolour;
    FontColour_s wbcolour2;
    FontWindowBorderStyle wbstyle;
    float wbsizex;
    float wbsizey;
    float wbpaddingx;
    float wbpaddingy;
    float mtop;
    float mbot;
    FontAlign align;
    unsigned __int8 fixFontScale;
};

union $B74F65445D7EF71299185E25121A140F
{
    $0F7D51B0C62DE9633A647472564C74BC __s0;
    FontStack_s stack;
};

struct FontState_s
{
    $B74F65445D7EF71299185E25121A140F ___u0;
    unsigned __int8* buffer;
    unsigned __int8* bufferMax;
    unsigned __int8* bufferCur;
    unsigned __int8* bufferEnd;
    int bufferSize;
    float upw;
    float uph;
};

int fontBuildMapIndex(
    const char* str,
    char          flags,
    short* charMapIndex,
    int           charMapBufferSizeIn,
    FontState_s* fs);

void fontInitialise(int FontBufferSize);

void fontRender();

float fontGetHeight();

void fontInitialiseState(FontState_s* state);

void fontSetWindow(float x, float y, float w, float h);

void AddFontCommand(void* data, unsigned int size);

void fontSetAlignment(FontAlign align);

void fontSetFixed(float f);

void fontSetFlag(unsigned int flag);

void fontSet(unsigned int handle);

void fontPrint(const char* format, ...);

void AddFontCommandText(void* data, unsigned int size, void* text, unsigned int tsize);

int fontFindLoadedFont(const char* inFontAsse);

void statusProject();

extern float flt_82B1036C;
extern FontState_s fontState;