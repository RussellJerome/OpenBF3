#include "Font.h"
#include "mem.h"
#include "util/unorgtypes.h"
#include "platform_pc/hw.h"

#include "engine/engineinit.h"


struct keyCodeData_s
{
    __int16 controlKey;
    vec4_u pos;
    vec4_u colour;
};

FontList_s fontArray[16];

FontState_s fontState;

FontState_s fontStateImmediate;

int fontStackPos = 0;
FontStack_s fontStack[4];

__int16(__cdecl* s_keycodeCallback)(__int16) = NULL;

void fontInitialiseState(FontState_s* state)
{
    int vpWidth = s_toSend.values.viewport.i[2];
    int vpHeight = s_toSend.values.viewport.i[3];

    state->___u0.stack.x = 0.0f;
    state->___u0.stack.y = 0.0f;
    state->___u0.stack.wx = 0.0f;
    state->___u0.stack.wy = 0.0f;
    state->___u0.stack.ww = 1.0f;
    state->___u0.stack.wh = 1.0f;

    state->___u0.stack.handle = 0;
    state->___u0.stack.cx = 0;
    state->___u0.stack.cy = 0;

    state->___u0.stack.cz = (int)windowfullscreen.width;
    state->___u0.stack.cw = (int)windowfullscreen.height;

    state->___u0.stack.wbcolour.r = 1.0f;
    state->___u0.stack.wbcolour.g = 1.0f;
    state->___u0.stack.wbcolour.b = 1.0f;
    state->___u0.stack.wbcolour.a = 1.0f;

    state->___u0.stack.wbcolour2.r = 0.0f;
    state->___u0.stack.wbcolour2.g = 0.0f;
    state->___u0.stack.wbcolour2.b = 0.0f;
    state->___u0.stack.wbcolour2.a = 1.0f;

    state->___u0.stack.wbstyle = (FontWindowBorderStyle)0;
    state->___u0.stack.wbsizex = (vpWidth > 0) ? 1.0f / (float)vpWidth : 0.0f;
    state->___u0.stack.wbsizey = (vpHeight > 0) ? 1.0f / (float)vpHeight : 0.0f;
    state->___u0.stack.wbpaddingx = (vpWidth > 0) ? 1.0f / (float)vpWidth : 0.0f;
    state->___u0.stack.wbpaddingy = (vpHeight > 0) ? 1.0f / (float)vpHeight : 0.0f;

    state->___u0.stack.mtop = 0.0f;
    state->___u0.stack.mbot = 0.0f;
    state->___u0.stack.align = (FontAlign)0;
    state->___u0.stack.fixedw = 0.0f;
    state->___u0.stack.flag = 0;

    state->___u0.stack.pw = 0.0f;
    state->___u0.stack.ph = 0.0f;

    state->___u0.stack.fixFontScale = 1;

    state->___u0.stack.t = 0.050000001f;
    state->___u0.stack.s = 1.0f;
    state->upw = 0.00125f;
    state->uph = 0.0016666667f;

    state->___u0.stack.colour.r = 1.0f;
    state->___u0.stack.colour.g = 1.0f;
    state->___u0.stack.colour.b = 1.0f;
    state->___u0.stack.colour.a = 1.0f;

    state->___u0.stack.colourOutline.r = 0.0f;
    state->___u0.stack.colourOutline.g = 0.0f;
    state->___u0.stack.colourOutline.b = 0.0f;
    state->___u0.stack.colourOutline.a = 1.0f;

    state->___u0.stack.colourUnderline.r = 1.0f;
    state->___u0.stack.colourUnderline.g = 1.0f;
    state->___u0.stack.colourUnderline.b = 1.0f;
    state->___u0.stack.colourUnderline.a = 1.0f;
}

void fontPopState(FontState_s* state, unsigned __int8 keepCurrentCursorPosition)
{
    if (fontStackPos <= 0)
        return;

    // Save current cursor position
    float x = state->___u0.stack.x;
    float y = state->___u0.stack.y;

    // Pop from stack by copying 0xC0 bytes
    --fontStackPos;
    memcpy(&state->___u0.stack, &fontStack[fontStackPos], 0xC0);

    // Re-select font texture
    FontHeader_s* header = state->___u0.stack.header;
    if (header)
        texSelectTextureEx(header->FontTexHandle, TEX_FUNC_MODULATE_RGBA);

    // Restore cursor position if keepCurrentCursorPosition
    state->___u0.stack.x = x;
    state->___u0.stack.y = y;
}

void fontInitialise(int FontBufferSize)
{
    fontStackPos = 0;
    memset(fontArray, 0, 0x4C0);
    memset(&fontState, 0, 0xDC);

    fontInitialiseState(&fontState);
    fontInitialiseState(&fontStateImmediate);

    fontState.bufferSize = FontBufferSize;

    unsigned __int8* v3 = nullptr;
    if (FontBufferSize > 0)
        v3 = (unsigned __int8*)memAllocAlignCore(
            FontBufferSize, 0, 0, __FILE__, 2739, nullptr, 1);

    fontState.buffer = v3;
    fontState.bufferMax = v3 + fontState.bufferSize;
    fontState.bufferCur = v3;
    fontState.bufferEnd = v3;

    fontState.upw = 0.00125f;
    fontState.uph = 0.0016666667f;

    fontStateImmediate.upw = 0.00125f;
    fontStateImmediate.uph = 0.0016666667f;

    fontState.___u0.stack.pw = fontState.___u0.stack.s * 0.00125f;
    fontState.___u0.stack.ph = fontState.___u0.stack.s * 0.0016666667f;

    fontStateImmediate.___u0.stack.pw = fontStateImmediate.___u0.stack.s * 0.00125f;
    fontStateImmediate.___u0.stack.ph = fontStateImmediate.___u0.stack.s * 0.0016666667f;
}

float fontPrintHW_MaxX = 0.0;
float fontPrintHW_MaxWidth = 0.0;

void fontDrawWinHW(FontState_s* state, float r, float g, float b, float a)
{
    FontStack_s& s = state->___u0.stack;

    float wx = s.wx;
    float wx2 = s.wx + s.ww;
    float wy = s.wy;
    float wy2 = s.wy + state->uph + s.wh;

    // Visibility check
    if (wx2 <= wx || wy2 <= wy)  return;
    if (wx > 1.0f || wx2 < 0.0f) return;
    if (wy > 1.0f || wy2 < 0.0f) return;

    // Clamp to screen
    if (wx < 0.0f) wx = 0.0f;
    if (wx2 > 1.0f) wx2 = 1.0f;
    if (wy < 0.0f) wy = 0.0f;
    if (wy2 > 1.0f) wy2 = 1.0f;

    // Apply padding
    float x1 = wx - s.wbpaddingx;
    float x2 = wx2 + s.wbpaddingx;
    float y1 = wy - s.wbpaddingy;
    float y2 = wy2 + s.wbpaddingy;

    texSelectTextureEx((unsigned int)-1, TEX_FUNC_MODULATE_RGBA);

    if (s.wbstyle != FONT_WINDOW_BORDER_NONE)
    {
        // Push render state for border drawing
        if (s_topIndex + 1 < 16)
        {
            ++s_topIndex;
            ++s_top;
            g_rState = &s_top->values;
        }

        // Select border colour based on style
        vec4_u col1, col2;

        if (s.wbstyle == FONT_WINDOW_BORDER_SOLID ||
            s.wbstyle == FONT_WINDOW_BORDER_INSET)
        {
            col1.v[0] = s.wbcolour.r;
            col1.v[1] = s.wbcolour.g;
            col1.v[2] = s.wbcolour.b;
            col1.v[3] = s.wbcolour.a;
        }
        else // OUTSET
        {
            col1.v[0] = s.wbcolour2.r;
            col1.v[1] = s.wbcolour2.g;
            col1.v[2] = s.wbcolour2.b;
            col1.v[3] = s.wbcolour2.a;
        }

        float bsx = s.wbsizex;
        float bsy = s.wbsizey;

        // Top border quad
        {
            vec2_u pts[4];
            pts[0] = { x2,        y1 };
            pts[1] = { x2,        y1 - bsy };
            pts[2] = { x2 + bsx,  y1 };
            pts[3] = { x2 + bsx,  y2 + bsy };
            idrawPolygon2D(&col1, 4, pts);
        }

        // Bottom border quad
        {
            vec2_u pts[4];
            pts[0] = { x1,        y2 };
            pts[1] = { x2,        y2 };
            pts[2] = { x2 + bsx,  y2 + bsy };
            pts[3] = { x1 - bsx,  y2 + bsy };
            idrawPolygon2D(&col1, 4, pts);
        }

        // Select second border colour
        if (s.wbstyle == FONT_WINDOW_BORDER_OUTSET)
        {
            col2.v[0] = s.wbcolour.r;
            col2.v[1] = s.wbcolour.g;
            col2.v[2] = s.wbcolour.b;
            col2.v[3] = s.wbcolour.a;
        }
        else if (s.wbstyle == FONT_WINDOW_BORDER_INSET)
        {
            col2.v[0] = s.wbcolour2.r;
            col2.v[1] = s.wbcolour2.g;
            col2.v[2] = s.wbcolour2.b;
            col2.v[3] = s.wbcolour2.a;
        }
        else
        {
            col2 = col1;
        }

        if (s.wbstyle != FONT_WINDOW_BORDER_SOLID)
        {
            // Left border quad
            {
                vec2_u pts[4];
                pts[0] = { x1 - bsx, y1 - bsy };
                pts[1] = { x1,        y1 };
                pts[2] = { x1,        y2 };
                pts[3] = { x1 - bsx,  y2 + bsy };
                idrawPolygon2D(&col2, 4, pts);
            }

            // Right border quad
            {
                vec2_u pts[4];
                pts[0] = { x1 - bsx, y1 - bsy };
                pts[1] = { x1,        y1 };
                pts[2] = { x2,        y1 };
                pts[3] = { x2 + bsx,  y1 - bsy };
                idrawPolygon2D(&col2, 4, pts);
            }
        }

        rStatePop();
    }

    // Draw filled rectangle
    dlPush2D();
    dlFillRectangleZ(x1, y1, x2, y2, 0.0f, r, g, b, a);
    platformPopMatricesHW();
    rStatePop();
}

void(__cdecl* s_keycodeRenderCallback)(void*, keyCodeData_s*, int) = NULL;
void(__cdecl* s_keycodeDimensionsCallback)(windowdef_s*, vec4_u*, float, float, float) = NULL;

FontColour_s* fontColourPacked16ABGR(FontColour_s* result, unsigned short abgr)
{
    // abgr packed as 4 bits per channel: AAAA BBBB GGGG RRRR
    unsigned int a = (abgr >> 12) & 0xF;
    unsigned int b = (abgr >> 8) & 0xF;
    unsigned int g = (abgr >> 4) & 0xF;
    unsigned int r = (abgr >> 0) & 0xF;

    const float inv = 1.0f / 15.0f; // 0.06666667f

    result->r = (float)r * inv;
    result->g = (float)g * inv;
    result->b = (float)b * inv;
    result->a = (float)a * inv;

    return result;
}

void fontClipChar(
    dlContextStruct* dc,
    vec4_u* col,
    float sx, float sy,
    float sw, float sh,
    float mx, float my,
    float mw, float mh,
    FontState_s* state)
{
    // Italic slant offset -- bit 0 of flag enables it
    float slant = 0.0f;
    if (state->___u0.stack.flag & 1)
        slant = sh * radius;

    // Pack colour to ARGB byte
    unsigned char ca = (unsigned char)(col->v[3] * 255.0f);
    unsigned char cr = (unsigned char)(col->v[0] * 255.0f);
    unsigned char cg = (unsigned char)(col->v[1] * 255.0f);
    unsigned char cb = (unsigned char)(col->v[2] * 255.0f);
    unsigned int  packed = ((unsigned int)ca << 24)
        | ((unsigned int)cr << 16)
        | ((unsigned int)cg << 8)
        | ((unsigned int)cb << 0);

    float x2 = mx + mw;
    float y2 = my + mh;

    float* p = dc->ptr;

    // Vertex layout per vert: u, v, colour(int), x, y
    // 5 verts total = 4 unique + 1 repeated for strip

    // Vertex 0: top-left of map quad
    p[0] = mx;
    p[1] = my;
    *(unsigned int*)(p + 2) = packed;
    dc->ptr = p + 2;

    // Vertex 1: top-left of screen quad (with slant)
    p = dc->ptr + 1;
    p[0] = sx + slant;
    p[1] = sy;
    p[2] = x2;
    p[3] = my;
    *(unsigned int*)(p + 4) = packed;
    dc->ptr = p + 3;

    // Vertex 2: top-right of screen quad (with slant)
    p = dc->ptr + 1;
    p[0] = sx + slant + sw;
    p[1] = sy;
    p[2] = x2;
    p[3] = y2;
    *(unsigned int*)(p + 4) = packed;
    dc->ptr = p + 4;

    // Vertex 3: bottom-left of screen quad
    p = dc->ptr + 1;
    p[0] = sx + sw;
    p[1] = sy + sh;
    p[2] = mx;
    p[3] = y2;
    *(unsigned int*)(p + 4) = packed;
    dc->ptr = p + 3;

    // Vertex 4: bottom-left repeated
    p = dc->ptr + 1;
    p[0] = sx;
    p[1] = sy + sh;
    dc->ptr = p + 2;

    // Update max X
    if (sx + sw > fontPrintHW_MaxX)
        fontPrintHW_MaxX = sx + sw;
}

int fontUC2Index(FontHeader_s* header, int code)
{
    if (!header)
        return -1;

    int numTrans = header->numTrans;
    if (numTrans <= 0)
        return -1;

    for (int i = 0; i < numTrans; i++)
    {
        FontTransBlk_s* blk = &header->transtable[i];
        int rangeEnd = blk->UniCodeBase + blk->Range;

        if (code < rangeEnd)
        {
            int idx = code - blk->UniCodeBase;
            if (idx < 0)
                return -1;
            return blk->MapIndexBase + idx;
        }
    }

    return -1;
}

unsigned short* fontStringBreak(FontState_s* state, unsigned short* cur, float* width)
{
    FontStack_s& s = state->___u0.stack;

    unsigned int savedFlag = s.flag;

    float totalWidth = 0.0f;
    float lastBreakWidth = 0.0f;
    float lastCharWidth = 0.0f;

    unsigned short* lastBreakPos = cur;
    unsigned short* pos = cur;

    // Get indices for backslash and forward slash (word break chars)
    int slashIdx = fontUC2Index(s.header, '\\');
    int fwdSlashIdx = fontUC2Index(s.header, '/');

    // Line height
    float lineHeight = (float)s.header->ymax * s.ph;

    // Space width
    float spaceWidth = (s.fixedw > 0.0f)
        ? s.pw * s.fixedw
        : fontArray[s.handle].spaceWidth * s.pw;

    // Available width for this line
    float ww = 0.0f;
    switch (s.align)
    {
    case FONT_ALIGN_LEFT:
    case FONT_ALIGN_FULL:
        ww = s.ww - (s.x - s.wx);
        break;
    case FONT_ALIGN_CENTRE:
    case FONT_ALIGN_RIGHT:
        ww = s.ww;
        break;
    }

    if (*cur == 0xFFFF)
        goto done;

    while (*pos != 0xFFFF)
    {
        short ch = *(short*)pos;

        if (ch == -2) // newline -- stop here
            break;

        if (ch == -3 || ch == -4 || ch == slashIdx || ch == fwdSlashIdx)
        {
            // Word-break eligible chars
            float charW = spaceWidth;

            if (ch == slashIdx || ch == fwdSlashIdx)
            {
                // Slash chars use glyph width
                if (s.fixedw > 0.0f)
                    charW = s.pw * s.fixedw;
                else
                    charW = (float)s.header->fontMaps[ch].fw * s.pw;
            }
            else if (ch == -4)
            {
                // Tab
                if (s.t > 0.0f)
                    charW = (floorf((s.x - s.wx + totalWidth) / s.t) + 1.000001f) * s.t
                    - (s.x - s.wx) - totalWidth;
                else
                    charW = spaceWidth;
            }

            if (s.flag & 8) charW += s.pw; // shadow

            bool fits = (s.flag & 0x40) != 0
                || (totalWidth <= ww && totalWidth + charW <= ww);

            if (fits)
            {
                totalWidth += charW;
                if (s.flag & 8) totalWidth += s.pw;
                lastBreakWidth = totalWidth;
                lastBreakPos = (unsigned short*)pos;
                lastCharWidth = charW;
            }
            else
            {
                // Doesn't fit
                if (totalWidth < ww)
                    goto wrap;
                if (lastBreakPos != (unsigned short*)cur)
                    goto trim;
                if (s.align == FONT_ALIGN_LEFT && s.x > s.wx && totalWidth < s.ww)
                {
                    // Wrap to next line
                    ww = s.ww;
                    s.y += lineHeight;
                    s.x = s.wx;
                    pos--;
                }
            }
        }
        else if (ch < -14 && ch > -50)
        {
            // Keycode glyph
            if (s_keycodeDimensionsCallback)
            {
                float kx = s.x;
                if (s.fixedw > 0.0f)
                    kx += (s.pw * s.fixedw - (float)s.header->bmWidth * s.pw) * 0.5f;

                vec4_u dims;
                s_keycodeDimensionsCallback(s.wdef, &dims, kx, s.y + s.ph, lineHeight);
                totalWidth += dims.v[2];
            }
        }
        else if (ch < 0)
        {
            // Control codes
            switch (ch)
            {
            case -10: s.flag ^= 2; break;
            case -12: s.flag ^= 8; break;
            case -5:
            case -51:
            case -6:
            case -7:
                pos++; // these codes carry an extra word
                break;
            }
        }
        else if (ch >= 0)
        {
            // Regular glyph
            float charW;
            if (s.fixedw > 0.0f)
            {
                charW = s.pw * s.fixedw;
            }
            else
            {
                charW = (float)s.header->fontMaps[ch].xadv * s.pw;

                if (s.header->kerntable)
                {
                    short nextCh = *(short*)(pos + 1);
                    if (nextCh >= 0)
                    {
                        int kern = s.header->kerntable[s.header->numMaps * ch + nextCh];
                        charW += (float)kern * s.pw * 0.25f;
                    }
                }
            }

            if (s.flag & 2) charW += s.pw; // bold
            if (s.flag & 8) charW += state->upw; // shadow

            totalWidth += charW;
            if (s.flag & 8) totalWidth += state->upw;
        }

        pos++;
    }

    // End of string -- check if it overflowed
    {
        unsigned int f = s.flag;
        if ((f & 0x20) || (f & 0x40) || totalWidth <= ww)
            goto done_set;

        if (lastBreakPos == (unsigned short*)cur)
        {
            if (s.align == FONT_ALIGN_LEFT && s.x > s.wx && totalWidth < s.ww)
            {
                s.x = s.wx;
                s.y += lineHeight;
                goto done_set;
            }
            goto done_set;
        }

        goto trim;
    }

wrap:
    lastBreakPos = (unsigned short*)pos;
    lastBreakWidth = totalWidth;
    goto done_width;

trim:
    lastBreakWidth -= lastCharWidth;
    goto done_width;

done_set:
    lastBreakPos = (unsigned short*)pos;
    lastBreakWidth = totalWidth;
    goto done_width;

done:
    lastBreakPos = (unsigned short*)pos;
    lastBreakWidth = totalWidth;

done_width:
    *width = lastBreakWidth;
    s.flag = savedFlag;
    return lastBreakPos;
}

void fontPrintHW(
    FontState_s* state,
    unsigned short* str,
    int               fontPrintFlags,
    float* sizeX,
    float* sizeY,
    float* outX,
    float* outY)
{
    FontStack_s& s = state->___u0.stack;

    bool doDraw = !(fontPrintFlags & 8);   // bit 3 clear = draw
    bool doScissor = !(fontPrintFlags & 0x10); // bit 4 clear = scissor
    bool doMeasure = !doDraw;

    float savedX = s.x;
    float savedY = s.y;

    fontPrintHW_MaxX = 0.0f;
    fontPrintHW_MaxWidth = 0.0f;

    float scaleX = 1.0f;

    // --- Pre-pass for scaling (FONT_ALIGN_FULL) ---
    if (doDraw && (s.flag & 0x40) != 0)
    {
        float ww = 0.0f;
        switch (s.align)
        {
        case FONT_ALIGN_LEFT:
        case FONT_ALIGN_FULL:
            ww = s.ww - (s.x - s.wx);
            break;
        case FONT_ALIGN_CENTRE:
        case FONT_ALIGN_RIGHT:
            ww = s.ww;
            break;
        }

        float measuredWidth = 0.0f;
        fontPrintHW(state, str, fontPrintFlags | 0x18, &measuredWidth, nullptr, nullptr, nullptr);

        if (measuredWidth > 0.0f)
            scaleX = min(1.0f, ww / measuredWidth);
    }

    // Allocate keycode data buffer if drawing
    keyCodeData_s* keycodeData = nullptr;
    if (doDraw)
        keycodeData = (keyCodeData_s*)memAllocAlignCore(0xB4, g_scratchStack, 0, "source/text/font.c", 1291, nullptr, 1);

    // --- Compute pixel scale ---
    windowdef_s* wdef = s.wdef;
    if (wdef)
    {
        float wdefW = wdef->right - wdef->left;
        int screenW = (int)windowfullscreen.width;
        state->upw = (1.0f / wdefW) / (float)screenW * scaleX;

        float wdefH = wdef->bottom - wdef->top;
        int screenH = (int)windowfullscreen.height;
        state->uph = (1.0f / wdefH) / (float)screenH;
    }
    else
    {
        int screenW = (int)windowfullscreen.width;
        state->upw = scaleX / (float)screenW;

        int screenH = (int)windowfullscreen.height;
        state->uph = 1.0f / (float)screenH;
    }

    // --- Compute font pw/ph ---
    if (!s.header)
    {
        if (sizeX) *sizeX = 0.0f;
        if (sizeY) *sizeY = 0.0f;
        memFreeFlags((char*)keycodeData, 1);
        return;
    }

    if (s.fixFontScale)
    {
        s.pw = s.s * state->upw;
        s.ph = s.s * state->uph;
    }
    else
    {
        if (wdef)
        {
            float wdefW = wdef->right - wdef->left;
            int   screenW = (int)windowfullscreen.width;
            s.pw = (1.0f / (wdefW * (float)screenW)) * s.s;
        }
        else
        {
            int screenH = (int)windowfullscreen.height;
            int screenW = (int)windowfullscreen.width;
            s.pw = ((float)screenH * s.s) / ((float)screenW * 600.0f);
        }
        s.ph = s.s * 0.0016666667f; // 1/600
    }

    float lineHeight = (float)s.header->ymax * s.ph;

    // Save colours for restore at end
    FontColour_s savedColour = s.colour;
    FontColour_s savedColourOutline = s.colourOutline;
    FontColour_s savedColourUnderline = s.colourUnderline;
    unsigned int savedFlag = s.flag;

    // Setup draw context
    if (doDraw)
    {
        if (doScissor)
        {
            platformScissorGL(s.cx, s.cy, s.cz - s.cx, s.cw - s.cy);
            dlScissorTest(1);
        }

        texSelectTextureEx(s.header->FontTexHandle, TEX_FUNC_MODULATE_RGBA);
        idlContextStruct.data = &idlContextData;
        idlContextStruct.ptr = nullptr;
    }

    bool colourSaved = false;
    FontColour_s savedColourInline;
    int keycodeCount = 0;

    // --- Main string loop ---
    unsigned short* cur = str;

    while (*cur != 0xFFFF)
    {
        // Line break pass
        float lineWidth = 0.0f;
        unsigned short* lineEnd = fontStringBreak(state, cur, &lineWidth);

        if (lineWidth > fontPrintHW_MaxWidth)
            fontPrintHW_MaxWidth = lineWidth;

        // Alignment
        float justifySpaceExtra = 0.0f;
        float justifyKerningExtra = 0.0f;

        switch (s.align)
        {
        case FONT_ALIGN_CENTRE:
            s.x = s.wx + (s.ww - lineWidth * scaleX) * 0.5f;
            break;
        case FONT_ALIGN_RIGHT:
            s.x = s.wx + s.ww - lineWidth * scaleX;
            break;
        case FONT_ALIGN_FULL:
        {
            float available = s.ww - (s.x - s.wx);
            if (lineEnd > (unsigned short*)cur && lineWidth > available * 0.60000002f)
            {
                // Count spaces and non-space chars for justification
                int spaces = 0, chars = 0;
                for (short* p = (short*)cur; p < (short*)lineEnd; p++)
                {
                    if (*p == -3) spaces++;
                    else if (*p >= 0) chars++;
                }

                float remaining = available - lineWidth;
                if (chars > 1)
                {
                    justifySpaceExtra = remaining / (float)(chars - 1);
                    if (justifySpaceExtra > s.pw)
                        justifySpaceExtra = s.pw;
                }

                float netRemaining = remaining - (float)(chars - 1) * justifySpaceExtra;
                if (netRemaining > 0.0f && spaces > 0)
                    justifyKerningExtra = netRemaining / (float)spaces;
            }
            break;
        }
        default:
            break;
        }

        // --- Per character loop ---
        short ch = *(short*)cur;
        if (ch == -1)
            goto next_line;

        while (cur <= lineEnd)
        {
            switch (ch)
            {
            case -4: // Tab
                if (s.t > 0.0f)
                {
                    float tabX = floorf((s.x - s.wx) / s.t);
                    s.x = (tabX + 1.000001f) * s.t + s.wx;
                }
                break;

            case -5: // Colour tag (4-bit RGBA packed)
            {
                unsigned short packed = cur[1];
                if (!colourSaved)
                {
                    colourSaved = true;
                    savedColourInline = s.colour;
                }
                if ((s.flag & 0x80) == 0)
                {
                    float inv = 1.0f / 15.0f;
                    s.colour.r = (float)((packed >> 8) & 0xF) * inv;
                    s.colour.g = (float)((packed >> 4) & 0xF) * inv;
                    s.colour.b = (float)((packed >> 0) & 0xF) * inv;
                    s.colour.a = (float)((packed >> 12) & 0xF) * inv;
                }
                cur++;
                break;
            }

            case -51: // Colour tag (4-bit, preserve alpha)
            {
                unsigned short packed = cur[1];
                if (!colourSaved)
                {
                    colourSaved = true;
                    savedColourInline = s.colour;
                }
                if ((s.flag & 0x80) == 0)
                {
                    float inv = 1.0f / 15.0f;
                    float savedA = s.colour.a;
                    s.colour.r = (float)((packed >> 8) & 0xF) * inv;
                    s.colour.g = (float)((packed >> 4) & 0xF) * inv;
                    s.colour.b = (float)((packed >> 0) & 0xF) * inv;
                    s.colour.a = savedA;
                }
                cur++;
                break;
            }

            case -6: // Outline colour packed
                if ((s.flag & 0x80) == 0)
                {
                    FontColour_s tmp;
                    fontColourPacked16ABGR(&tmp, cur[1]);
                    s.colourOutline = tmp;
                }
                cur++;
                break;

            case -7: // Underline colour packed
                if ((s.flag & 0x80) == 0)
                {
                    FontColour_s tmp;
                    fontColourPacked16ABGR(&tmp, cur[1]);
                    s.colourUnderline = tmp;
                }
                cur++;
                break;

            case -8: // Restore colour
                if (colourSaved && (s.flag & 0x80) == 0)
                    s.colour = savedColourInline;
                colourSaved = false;
                break;

            case -2: // Newline
                s.x = s.wx;
                s.y += lineHeight;
                break;

            case -13: // Small newline (half line)
                s.y += lineHeight * 0.5f;
                break;

            case -10: s.flag ^= 2; break; // Toggle bold
            case -9:  s.flag ^= 1; break; // Toggle italic
            case -11: s.flag ^= 4; break; // Toggle underline
            case -12: s.flag ^= 8; break; // Toggle shadow

            case -3: // Space
            {
                float spaceW = (s.fixedw > 0.0f)
                    ? s.pw * s.fixedw
                    : fontArray[s.handle].spaceWidth * s.pw;
                if (s.flag & 8) spaceW += s.pw;
                s.x += spaceW + justifyKerningExtra;
                break;
            }

            default:
                if (ch < -14 && ch > -50)
                {
                    // Keycode glyph
                    if (s_keycodeRenderCallback && s_keycodeDimensionsCallback)
                    {
                        float sx = s.x;
                        float sy = s.y + s.ph;
                        if (s.fixedw > 0.0f)
                            sx += (s.pw * s.fixedw - s.pw * (float)s.header->bmWidth) * 0.5f;

                        vec4_u dims;
                        s_keycodeDimensionsCallback(s.wdef, &dims, sx, sy, lineHeight);
                        s.x += dims.v[2];

                        if (doDraw && keycodeCount < 5)
                        {
                            keyCodeData_s* kd = &keycodeData[keycodeCount++];
                            kd->controlKey = (__int16)*cur;
                            kd->pos.v[0] = dims.v[0];
                            kd->pos.v[1] = dims.v[1];
                            kd->pos.v[2] = dims.v[2];
                            kd->pos.v[3] = dims.v[3];
                            kd->colour.v[0] = s.colour.r;
                            kd->colour.v[1] = s.colour.g;
                            kd->colour.v[2] = s.colour.b;
                            kd->colour.v[3] = s.colour.a;
                        }
                    }
                }
                else if (ch >= 0)
                {
                    // Regular glyph
                    FontMap_s* fm = &s.header->fontMaps[ch];

                    float bmW = (float)s.header->bmWidth;
                    float bmH = (float)s.header->bmHeight;
                    float u1 = (float)fm->fx / bmW;
                    float v1 = (float)fm->fy / bmH;
                    float u2 = (float)(fm->fx + fm->fw) / bmW;
                    float v2 = (float)(fm->fy + fm->fh) / bmH;
                    float gw = (float)fm->fw * s.pw * scaleX;
                    float gh = (float)fm->fh * s.ph;
                    float xoff = (float)fm->xoff * s.ph;
                    float yoff = (float)fm->yoff * s.ph;
                    float xadv = (float)fm->xadv;

                    float charW;
                    if (s.fixedw > 0.0f)
                        charW = s.pw * s.fixedw;
                    else
                    {
                        charW = xadv * s.pw;
                        if (s.header->kerntable)
                        {
                            short nextCh = *(short*)(cur + 1);
                            if (nextCh >= 0)
                            {
                                int kern = s.header->kerntable[s.header->numMaps * ch + nextCh];
                                charW += (float)kern * s.pw * 0.25f;
                            }
                        }
                    }

                    if (s.flag & 2) charW += s.pw; // Bold
                    if (s.flag & 8) charW += state->upw; // Shadow

                    float sx = s.x;
                    if (s.fixedw > 0.0f)
                        sx += (s.pw * s.fixedw - gw) * 0.5f;
                    else
                        sx += (float)fm->xoff * s.pw;

                    float sy = (float)fm->yoff * s.ph + s.y;
                    if (s.flag & 8) sy -= state->uph;

                    if (doDraw && initData.graphicsEnable)
                    {
                        // Calculate number of vertices needed
                        int numVerts = 1;
                        if (s.flag & 8) numVerts = (s.flag & 2) ? 7 : 5;
                        if (s.flag & 2) numVerts++;

                        float* vb = dlBeginMain(&idlContextStruct, 6, numVerts * 4, 24);
                        idlContextStruct.ptr = vb;
                        if (vb)
                        {
                            if (s.flag & 8)
                            {
                                fontClipChar(&idlContextStruct, (vec4_u*)&s.colourOutline,
                                    sx - state->upw, sy, gw, gh, u1, v1, u2, v2, state);
                                fontClipChar(&idlContextStruct, (vec4_u*)&s.colourOutline,
                                    sx, sy - state->uph, gw, gh, u1, v1, u2, v2, state);
                                fontClipChar(&idlContextStruct, (vec4_u*)&s.colourOutline,
                                    sx, state->uph + sy, gw, gh, u1, v1, u2, v2, state);

                                if (s.flag & 2)
                                {
                                    fontClipChar(&idlContextStruct, (vec4_u*)&s.colourOutline,
                                        sx + state->upw, sy - state->uph, gw, gh, u1, v1, u2, v2, state);
                                    fontClipChar(&idlContextStruct, (vec4_u*)&s.colourOutline,
                                        sx + state->upw, state->uph + sy, gw, gh, u1, v1, u2, v2, state);
                                    fontClipChar(&idlContextStruct, (vec4_u*)&s.colourOutline,
                                        sx + state->upw * 2.0f, sy, gw, gh, u1, v1, u2, v2, state);
                                }
                                else
                                {
                                    fontClipChar(&idlContextStruct, (vec4_u*)&s.colourOutline,
                                        sx + state->upw, sy, gw, gh, u1, v1, u2, v2, state);
                                }
                            }

                            fontClipChar(&idlContextStruct, (vec4_u*)&s.colour,
                                sx, sy, gw, gh, u1, v1, u2, v2, state);

                            if (s.flag & 2)
                                fontClipChar(&idlContextStruct, (vec4_u*)&s.colour,
                                    sx + s.pw, sy, gw, gh, u1, v1, u2, v2, state);

                            dlEndMain(idlContextStruct.data, idlContextStruct.ptr);
                        }

                        s.x += (charW + justifySpaceExtra) * scaleX;
                    }
                    break;
                }

                ch = *(short*)++cur;
                if (ch == -1)
                    break;
            }

        next_line:
            cur += 2; // advance past newline/end marker
            ch = *(short*)cur;
            if (*cur == 0xFFFF)
                break;
        }

        // --- Keycode render callback ---
        if (s_keycodeRenderCallback)
        {
            if (doDraw && keycodeCount > 0)
                s_keycodeRenderCallback(&idlContextStruct, keycodeData, keycodeCount);
        }

        // --- Cleanup scissor ---
        if (doDraw && doScissor)
        {
            // Restore full scissor
            float wx2 = min(s.wx + s.ww, 1.0f);
            float wy2 = min(s.wy + s.wh, 1.0f);
            float wx1 = max(s.wx, 0.0f);
            float wy1 = max(s.wy, 0.0f);

            int screenW = (int)windowfullscreen.width;
            int screenH = (int)windowfullscreen.height;
            int px = (int)((wx1 < 0.0f ? 0.0f : wx1) * (float)screenW);
            int py = (int)((wy1 < 0.0f ? 0.0f : wy1) * (float)screenH);
            int pw = (int)(max(wx2 - wx1, 0.0f) * (float)screenW);
            int ph = (int)(max(wy2 - wy1, 0.0f) * (float)screenH);

            platformScissorGL(0, 0, pw, ph);
            dlScissorTest(0);
        }

        if (doDraw)
            memFreeFlags((char*)keycodeData, 1);

        // --- Compute output height ---
        float outHeight = s.y - savedY;
        if (s.x > s.wx)
            outHeight += lineHeight;

        // Restore state
        s.flag = savedFlag;
        s.colour = savedColour;
        s.colourOutline = savedColourOutline;
        s.colourUnderline = savedColourUnderline;

        if (colourSaved)
            s.colour = savedColourInline; // shouldn't happen but safety

        if (sizeX) *sizeX = fontPrintHW_MaxWidth;
        if (sizeY) *sizeY = outHeight;

        if (!doDraw)
        {
            s.x = savedX;
            s.y = savedY;
        }
    }
}

void fontDrawSliderHW(
    FontState_s* state,
    float          w,
    float          factor,
    unsigned char  marked,
    float          marker,
    float          max)
{
    FontStack_s& s = state->___u0.stack;

    float x = s.x;
    float y = s.y;

    // Right align -- start from right edge
    if (s.align == FONT_ALIGN_RIGHT)
    {
        x = s.wx + s.ww - w;
        s.x = x;
    }

    // Advance cursor past slider
    s.x = x + w;

    float upw = state->upw;
    float uph = state->uph;

    float innerX = x + upw * 2.0f;
    float innerY = y + uph * 2.0f;
    float outerW = w - upw * 2.0f;

    float height = fontGetHeight();
    float outerH = -(upw * 4.0f - height);

    // Push render state
    if (s_topIndex + 1 < 16)
    {
        ++s_topIndex;
        ++s_top;
        g_rState = &s_top->values;
    }

    texSelectTextureEx((unsigned int)-1, TEX_FUNC_MODULATE_RGBA);
    dlScissorTest(1);

    // Scissor to window bounds
    {
        float winW = windowfullscreen.width;
        float winH = windowfullscreen.height;

        int sx = (int)(s.wx * winW);
        int sy = (int)(s.wy * winH);
        int ex = (int)((s.wx + s.ww) * winW);
        int ey = (int)((s.wy + s.wh) * winH);

        platformScissorGL(sx, sy, ex - sx, ey - sy);
    }

    // Outer border (outline colour)
    idrawQuad2D((const vec4_u*)&s.colourOutline,
        x, innerY, outerW, outerH);

    // Inner border (underline colour)
    float bx = x + upw;
    float by = innerY + uph;
    float bw = -(upw * 2.0f - outerW);
    float bh = -(uph * 2.0f - outerH);

    idrawQuad2D((const vec4_u*)&s.colourUnderline,
        bx, by, bw, bh);

    // Fill bar (main colour, scaled by factor)
    float fx = bx + upw;
    float fy = by + uph * 1.01f;
    float fw = -(upw * 2.0f - bw) * factor;
    float fh = -(uph * 2.0f - bh);

    idrawQuad2D((const vec4_u*)&s.colour,
        fx, fy, fw, fh);

    // Marker tick (if marked == 1)
    if (marked == 1)
    {
        vec4_u markerCol;
        markerCol.v[0] = 1.0f;
        markerCol.v[1] = 0.0f;
        markerCol.v[2] = 0.0f;
        markerCol.v[3] = 1.0f;

        float markerX = (marker / max) * w + fx - upw * 2.0f;

        idrawQuad2D(&markerCol,
            markerX, fy, 0.001f, fh);
    }

    rStatePop();
}

float fontGetHeight()
{
    FontStack_s& s = fontState.___u0.stack;

    if (s.handle == (unsigned int)-1)
        return 0.0f;

    FontHeader_s* font = fontArray[s.handle].font;
    if (!font)
        return 0.0f;

    float ymax = (float)font->ymax + 1.0f;
    return ymax * (s.s * 0.0016666667f);
}

void fontPushState(FontState_s* state)
{
    if (fontStackPos >= 4)
        return;

    memcpy(&fontStack[fontStackPos], &state->___u0.stack, 0xC0);
    fontStackPos++;
}

int fontInterpretCommand(FontCommand* Comm, FontState_s* state, int draw)
{
    // Comm is a buffer where first int is the command type
    // followed by variable data
    unsigned int* data = (unsigned int*)Comm;
    FontCommand cmd = (FontCommand)data[0];

    // Shorthand to access state fields via union
    FontStack_s& s = state->___u0.stack;

    switch (cmd)
    {
    case FontCommHandle:
    {
        unsigned int handle = data[1];
        s.handle = handle;
        if (handle == (unsigned int)-1)
        {
            s.header = nullptr;
        }
        else
        {
            FontHeader_s* font = fontArray[handle].font;
            s.header = font;
            if (font)
            {
                texSelectTextureEx(font->FontTexHandle, TEX_FUNC_MODULATE_RGBA);
                return 8;
            }
        }
        return 8;
    }

    case FontCommPosition:
        s.x = s.wx + ((float*)data)[1];
        s.y = s.wy + ((float*)data)[2];
        return 12;

    case FontCommColourHex:
    {
        unsigned int hex = data[1];
        const float inv = 1.0f / 255.0f;
        // ASM: extrwi extracts bytes in order r=16-23, g=8-15, b=0-7, a=24-31
        s.colour.r = (float)((hex >> 16) & 0xFF) * inv;
        s.colour.g = (float)((hex >> 8) & 0xFF) * inv;
        s.colour.b = (float)((hex >> 0) & 0xFF) * inv;
        s.colour.a = (float)((hex >> 24) & 0xFF) * inv;
        return 8;
    }

    case FontCommColour:
        s.colour.r = ((float*)data)[1];
        s.colour.g = ((float*)data)[2];
        s.colour.b = ((float*)data)[3];
        return 16;

    case FontCommOutColour:
        s.colourOutline.r = ((float*)data)[1];
        s.colourOutline.g = ((float*)data)[2];
        s.colourOutline.b = ((float*)data)[3];
        return 16;

    case FontCommUnderLineColour:
        s.colourUnderline.r = ((float*)data)[1];
        s.colourUnderline.g = ((float*)data)[2];
        s.colourUnderline.b = ((float*)data)[3];
        return 16;

    case FontCommAlpha:
        s.colour.a = ((float*)data)[1];
        return 8;

    case FontCommOutAlpha:
        s.colourOutline.a = ((float*)data)[1];
        return 8;

    case FontCommUnderLineAlpha:
        s.colourUnderline.a = ((float*)data)[1];
        return 8;

    case FontCommScale:
        s.s = ((float*)data)[1];
        return 8;

    case FontCommFixed:
        s.fixedw = ((float*)data)[1];
        return 8;

    case FontCommAlign:
        s.align = (FontAlign)data[1];
        return 8;

    case FontCommWindow:
        s.wx = ((float*)data)[1];
        s.wy = ((float*)data)[2];
        s.ww = ((float*)data)[3];
        s.wh = ((float*)data)[4];
        s.y = s.wy + s.y;
        s.x = s.wx + s.x;
        flt_82B1036C = 0.0f;
        return 20;

    case FontCommMargin:
        s.mtop = ((float*)data)[1];
        s.mbot = ((float*)data)[2];
        return 12;

    case FontCommPrint:
        fontPrintHW(state, (unsigned short*)&data[2], 0, 0, 0, 0, 0);
        if (fontPrintHW_MaxWidth > flt_82B1036C)
            flt_82B1036C = fontPrintHW_MaxX;
        return (int)data[1] + 8;

    case FontCommNewLine:
    {
        s.x = s.wx;
        float ymax = (float)s.header->ymax;
        float h = windowfullscreen.height;
        s.y = ((ymax + 1.0f) * (s.s / h)) + s.y;
        return 4;
    }

    case FontCommSmLine:
    {
        s.x = s.wx;
        float ymax = (float)s.header->ymax;
        float h = windowfullscreen.height;
        s.y = ((ymax * 0.25f + 1.0f) * (s.s / h)) + s.y;
        return 4;
    }

    case FontCommSetFlag:
        s.flag |= data[1];
        return 8;

    case FontCommUnsetFlag:
        s.flag &= ~data[1];
        return 8;

    case FontCommDrawWin:
        fontDrawWinHW(
            state,
            ((float*)data)[1],
            ((float*)data)[2],
            ((float*)data)[3],
            ((float*)data)[4]);
        return 20;

    case FontCommDefault:
        fontInitialiseState(&fontState);
        return 4;

    case FontCommSetTab:
        s.t = ((float*)data)[1];
        return 8;

    case FontCommBorderColour:
        s.wbcolour = *(FontColour_s*)&data[1];
        return 20;

    case FontCommBorderColour2:
        s.wbcolour2 = *(FontColour_s*)&data[1];
        return 20;

    case FontCommBorderStyle:
        s.wbstyle = (FontWindowBorderStyle)data[1];
        return 8;

    case FontCommBorderSize:
        s.wbsizex = ((float*)data)[1];
        s.wbsizey = ((float*)data)[2];
        return 12;

    case FontCommBorderPadding:
        s.wbpaddingx = ((float*)data)[1];
        s.wbpaddingy = ((float*)data)[2];
        return 12;

    case FontCommSlider:
    {
        // data layout: [cmd][w][factor][marker_byte+pad][max][strlen][str...]
        int strLen = (int)data[6];
        if (strLen > 0)
        {
            FontAlign savedAlign = s.align;
            float sizeX = 0.0f;
            fontPrintHW(state, (unsigned short*)&data[7], 8, &sizeX, 0, 0, 0);

            if (s.align == FONT_ALIGN_RIGHT)
            {
                s.align = FONT_ALIGN_LEFT;
                s.x = (s.ww + s.wx) - ((float*)data)[1] - sizeX;
            }

            fontPrintHW(state, (unsigned short*)&data[7], 0, 0, 0, 0, 0);
            s.align = savedAlign;
        }

        fontDrawSliderHW(
            state,
            ((float*)data)[1],          // w
            ((float*)data)[2],          // factor
            ((unsigned char*)data)[12], // marker byte
            ((float*)data)[4],          // max
            ((float*)data)[5]);         // (unused/extra)

        float width = s.x - s.wx;
        if (width > flt_82B1036C)
            flt_82B1036C = width;

        return strLen + 28;
    }

    case FontCommDimensionCallback:
    {
        void (*cb)(float, float) = (void(*)(float, float))data[1];
        cb(flt_82B1036C, s.y - s.wy);
        return 12;
    }

    case FontCommCustomRenderCallback:
    {
        void (*cb)(float, float, float, float) = (void(*)(float, float, float, float))data[1];
        cb(s.wx, s.wy, s.ww, s.wh);
        return 12;
    }

    case FontCommPush:
        fontPushState(state);
        return 4;

    case FontCommPop:
        fontPopState(state, 1);
        return 4;

    default:
        return 0;
    }
}

float flt_82B1036C = 0.0;
void fontRender()
{
    if (!initData.graphicsEnable)
        return;

    fontInitialiseState(&fontState);
    fontState.bufferEnd = fontState.bufferCur;
    flt_82B1036C = 0.0f;
    fontState.bufferCur = fontState.buffer;

    if (fontState.buffer < fontState.bufferEnd)
    {
        fontPreRenderHW(1);

        unsigned char* cur = fontState.bufferCur;
        while (cur < fontState.bufferEnd)
        {
            int size = fontInterpretCommand((FontCommand*)cur, &fontState, 1);
            cur += size;
            fontState.bufferCur = cur;
        }

        if (initData.graphicsEnable)
        {
            platformPopMatricesHW();
            rStatePop();
        }
    }

    fontInitialiseState(&fontState);
    fontState.bufferCur = fontState.buffer;
    fontState.bufferEnd = fontState.buffer;
    fontStackPos = 0;
}

void AddFontCommand(void* data, unsigned int size)
{
    if (fontState.bufferCur + size < fontState.bufferMax)
    {
        memcpy(fontState.bufferCur, data, size);
        fontState.bufferCur += size;
    }
}

void fontSetAlignment(FontAlign align)
{
    struct { unsigned int cmd; unsigned int align; } cmd;
    cmd.cmd = FontCommAlign;
    cmd.align = (unsigned int)align;
    AddFontCommand(&cmd, 8);
    fontState.___u0.stack.align = align;
}

void fontSetFixed(float f)
{
    struct { unsigned int cmd; float val; } cmd;
    cmd.cmd = FontCommFixed;
    cmd.val = f;
    AddFontCommand(&cmd, 8);
    fontState.___u0.stack.fixedw = f;
}

void fontSetFlag(unsigned int flag)
{
    struct { unsigned int cmd; unsigned int flag; } cmd;
    cmd.cmd = FontCommSetFlag;
    cmd.flag = flag;
    AddFontCommand(&cmd, 8);
    fontState.___u0.stack.flag |= flag;
}

void fontSet(unsigned int handle)
{
    struct { unsigned int cmd; unsigned int handle; } cmd;
    cmd.cmd = FontCommHandle;
    cmd.handle = handle;
    AddFontCommand(&cmd, 8);

    fontState.___u0.stack.handle = handle;
    fontState.___u0.stack.header = fontArray[handle].font;
}

void fontSetWindow(float x, float y, float w, float h)
{
    FontStack_s& s = fontState.___u0.stack;

    // Clamp w and h to >= 0
    float cw = (w < 0.0f) ? 0.0f : w;
    float ch = (h < 0.0f) ? 0.0f : h;

    // Emit FontCommWindow command into buffer
    struct { unsigned int cmd; float x, y, w, h; } cmd;
    cmd.cmd = FontCommWindow;
    cmd.x = x;
    cmd.y = y;
    cmd.w = cw;
    cmd.h = ch;
    AddFontCommand(&cmd, sizeof(cmd));

    // Set window on state directly
    s.wx = x;
    s.wy = y;
    s.ww = w;
    s.wh = h;

    // Compute pixel scissor rect
    int screenW = (int)windowfullscreen.width;
    int screenH = (int)windowfullscreen.height;

    s.cx = (int)((float)screenW * x);
    s.cy = (int)((float)screenH * y);
    s.cz = (int)(((float)x + (float)w) * (float)screenW);
    s.cw = (int)(((float)y + (float)h) * (float)screenH);

    // Advance cursor
    s.x += x;
    s.y += y;
}

void AddFontCommandText(void* data, unsigned int size, void* text, unsigned int tsize)
{
    if (fontState.bufferCur + size + tsize < fontState.bufferMax)
    {
        memcpy(fontState.bufferCur, data, size);
        fontState.bufferCur += size;
        memcpy(fontState.bufferCur, text, tsize);
        fontState.bufferCur += tsize;
    }
}

int UTF8Toutf8ValArray(
    const char* aSrc,
    unsigned int* aSrcLength,
    unsigned int* aDest,
    unsigned int* aDestLength)
{
    int           ok = 1;
    const char* srcEnd = aSrc + *aSrcLength;
    unsigned int* destEnd = aDest + *aDestLength;
    const char* src = aSrc;
    unsigned int* dst = aDest;
    int           remaining = 0;   // continuation bytes expected
    unsigned int  accum = 0;   // accumulator for multi-byte char
    unsigned char seqLen = 0;   // total bytes in current sequence

    while (src < srcEnd)
    {
        if (dst >= destEnd)
            break;

        unsigned char b = (unsigned char)*src;

        if (remaining > 0)
        {
            // Continuation byte
            if ((b & 0xC0) != 0x80)
            {
                // Invalid -- back up and fail
                src--;
                ok = 0;
                break;
            }

            int shift = 2 * (remaining - 1 + remaining - 1); // bits to shift
            // Proper shift: (remaining-1)*6 bits from the left of remaining continuation bytes
            // Each continuation byte adds 6 bits
            int bitsLeft = remaining - 1;
            accum |= (unsigned int)(b & 0x3F) << (6 * bitsLeft);
            remaining--;

            if (remaining == 0)
            {
                // Validate overlong sequences
                if (seqLen == 2 && accum < 0x80)     goto invalid;
                else if (seqLen == 3 && accum < 0x800)    goto invalid;
                else if (seqLen == 4 && accum < 0x10000)  goto invalid;
                else if (seqLen > 4)                      goto invalid;

                // Validate surrogate and max range
                if ((accum & 0xFFFFF800) == 0xD800)       goto invalid;
                if (accum > 0x10FFFF)                      goto invalid;

                if (accum <= 0xFFFF)
                {
                    // BOM skip
                    if (accum != 0xFEFF)
                        *dst++ = accum;
                }
                else
                {
                    // Encode as surrogate pair
                    unsigned int v = accum - 0x10000;
                    *dst++ = ((v >> 10) & 0x3FF) | 0xD800;
                    *dst++ = (v & 0x3FF) | 0xDC00;
                }

                accum = 0;
                remaining = 0;
                seqLen = 1;
            }
        }
        else
        {
            // Leading byte
            if ((b & 0x80) == 0)
            {
                // ASCII
                *dst++ = (int)(signed char)b;
                seqLen = 1;
            }
            else if ((b & 0xE0) == 0xC0)
            {
                accum = (unsigned int)(b & 0x1F) << 6;
                remaining = 1;
                seqLen = 2;
            }
            else if ((b & 0xF0) == 0xE0)
            {
                accum = (unsigned int)(b & 0x0F) << 12;
                remaining = 2;
                seqLen = 3;
            }
            else if ((b & 0xF8) == 0xF0)
            {
                accum = (unsigned int)(b & 0x07) << 18;
                remaining = 3;
                seqLen = 4;
            }
            else if ((b & 0xFC) == 0xF8)
            {
                accum = (unsigned int)(b & 0x03) << 24;
                remaining = 4;
                seqLen = 5;
            }
            else if ((b & 0xFE) == 0xFC)
            {
                accum = (unsigned int)(b & 0x01) << 30;
                remaining = 5;
                seqLen = 6;
            }
            else
            {
                goto invalid;
            }
        }

        src++;
        continue;

    invalid:
        ok = 0;
        break;
    }

    // If output full but input not exhausted
    if (ok && src < srcEnd && dst >= destEnd)
        ok = 0;

    // If sequence incomplete
    if (ok && remaining > 0)
        ok = 0;

    *aSrcLength = (unsigned int)(src - aSrc);
    *aDestLength = (unsigned int)(dst - aDest);
    return ok;
}

int fontBuildMapIndex(
    const char* str,
    char          flags,
    short* charMapIndex,
    int           charMapBufferSizeIn,
    FontState_s* fs)
{
    FontStack_s& s = fs->___u0.stack;

    // Convert UTF-8 string to UTF-32 value array
    unsigned int srcLen = 0x1000;
    unsigned int dstLen = 0x1000;
    unsigned int utf32[0x1000];
    UTF8Toutf8ValArray(str, &srcLen, utf32, &dstLen);

    int    count = 0;
    int    total = (int)srcLen - 1;
    short* out = charMapIndex;

    unsigned int* cur = utf32;
    unsigned int* curNext = utf32 + 1;

    if (total <= 0)
        goto done;

    while (count != total)
    {
        short ch = (short)*cur;

        if (ch == '|')
        {
            // Control sequence
            short next = (short)toupper((short)*curNext);
            cur++;
            curNext++;

            switch (next)
            {
            case '|':
            {
                // Literal pipe -- find '|' in font maps
                FontHeader_s* hdr = s.header;
                if (hdr && hdr->numMaps > 0)
                {
                    for (int i = 0; i < hdr->numMaps; i++)
                    {
                        if (hdr->fontMaps[i].code == '|')
                        {
                            *out++ = (short)i;
                            count++;
                            break;
                        }
                    }
                }
                break;
            }
            case 'T': *out++ = -4;  count++; break; // Tab
            case 'N': *out++ = -2;  count++; break; // Newline
            case 'G':
                out[0] = -2;
                out[1] = -13;
                out += 2;
                count += 2;
                break;
            case 'I': *out++ = -9;  count++; break; // Toggle italic
            case 'B': *out++ = -10; count++; break; // Toggle bold
            case 'U': *out++ = -11; count++; break; // Toggle underline
            case 'O': *out++ = -12; count++; break; // Toggle shadow

            case 'C':
            case 'K':
            case 'A':
            {
                short tag = next;
                cur++;
                curNext++;

                short v21 = (short)*cur;
                if (v21 == 'Z' || v21 == 'z')
                {
                    // End colour sequence
                    *out++ = -8;
                    count++;
                }
                else
                {
                    // Parse 4 hex digits into packed colour
                    unsigned short packed = 0;
                    int  nibbles = 0;
                    int  shift = 0;
                    bool hasAlpha = false;
                    unsigned int* p = cur;

                    while (nibbles < 4)
                    {
                        short c = (short)*p;
                        if (!c) break;

                        short u = (short)toupper(c);
                        unsigned short nibble;

                        if (u >= 'A' && u <= 'F')
                            nibble = (unsigned short)(u - 0x37);
                        else if (u == 'X')
                        {
                            nibble = 0;
                            hasAlpha = true;
                        }
                        else
                            nibble = (unsigned short)(u - '0');

                        packed |= (nibble << shift);
                        shift += 4;
                        nibbles++;
                        p++;
                    }

                    if (nibbles == 4)
                    {
                        cur += 3;
                        curNext += 3;

                        short cmd;
                        if (tag == 'A') cmd = -7;  // underline colour
                        else if (tag == 'K') cmd = -6;  // outline colour
                        else                 cmd = hasAlpha ? -51 : -5; // text colour

                        out[0] = cmd;
                        out[1] = (short)packed;
                        out += 2;
                        count += 2;
                    }
                }
                charMapIndex = charMapIndex; // restore (original restores r27)
                break;
            }
            default:
                break;
            }
        }
        else if (ch == '{')
        {
            // Keycode callback
            short upper = (short)toupper((short)*curNext);
            cur++;
            curNext++;

            if (s_keycodeCallback)
            {
                *out++ = s_keycodeCallback(upper);
                count++;
            }

            cur++;
            curNext++;
        }
        else if (ch == 0)
        {
            break;
        }
        else if (ch == '\n')
        {
            if (!(flags & 4))
            {
                *out++ = -2;
                count++;
            }
            else
            {
                break;
            }
        }
        else if (ch == ' ')
        {
            *out++ = -3;
            count++;
        }
        else if (ch == '\t')
        {
            *out++ = -4;
            count++;
        }
        else
        {
            // Regular character
            short c = ch;
            if (s.flag & 0x10)
                c = (short)toupper(c);

            int idx = fontUC2Index(s.header, c);
            if (idx != -1)
            {
                *out++ = (short)idx;
                count++;
            }
        }

        cur++;
        curNext++;
    }

done:
    // Terminate with -1, pad to even count
    charMapIndex[count] = -1;
    int result = count + 1;
    if (result & 1)
    {
        charMapIndex[result] = -1;
        result++;
    }
    return result;
}

void fontPrint(const char* format, ...)
{
    if (!format)
        return;

    // Format string into buffer
    char str[0x1000];
    va_list args;
    va_start(args, format);
    vsnprintf(str, sizeof(str), format, args);
    va_end(args);
    str[sizeof(str) - 1] = 0;

    // Convert to font char map indices
    unsigned short charMap[0x1000];
    int charCount = fontBuildMapIndex(str, 1, (short*)charMap, 0x1000, &fontState);

    // Emit FontCommPrint command with text data
    struct { unsigned int cmd; unsigned int tsize; } header;
    header.cmd = FontCommPrint;
    header.tsize = (unsigned int)(charCount * 2);
    AddFontCommandText(&header, 8, charMap, header.tsize);
}

int fontFindLoadedFont(const char* inFontAsset)
{
    // Search through fontArray (16 entries, each FontList_s is 0x4C bytes)
    for (int i = 0; i < 16; i++)
    {
        if (!fontArray[i].font)
            continue;

        // strcmp
        const char* a = inFontAsset;
        const char* b = fontArray[i].assetName;
        int diff = 0;
        do
        {
            diff = (unsigned char)*a - (unsigned char)*b;
            if (!*a) break;
            a++;
            b++;
        } while (!diff);

        if (!diff)
            return i;
    }

    return -1;
}

void statusProject()
{
    // Count prop types in scene
    int countNpcProp = 0;
    int countSshp = 0;
    int countHvrp = 0;
    int count6wvp = 0;
    int countRPbf = 0;
    int countHhgp = 0;
    int countExpp = 0;
    int countEvpr = 0;
    int totalVehicles = 0;

    //CGamePropIterator it;
    //it.m_index = s_propMgrState.activeList.m_list.count;
    //it.m_list = &s_propMgrState.activeList;

    //for (CGameProp* prop = CGamePropIterator::Next(&it);
    //    prop;
    //    prop = CGamePropIterator::Next(&it))
    //{
    //    if (prop->IsOfObjectType(0x6E706370) || prop->IsOfObjectType(0x6E706370))
    //        countNpcProp++;
    //    if (prop->IsOfObjectType(0x73736870))
    //        countSshp++;
    //    if (prop->IsOfObjectType(0x68767270))
    //        countHvrp++;
    //    if (prop->IsOfObjectType(0x36777670))
    //        count6wvp++;
    //    if (prop->IsOfObjectType(0x52506266) || prop->IsOfObjectType(0x52655072))
    //        countRPbf++;
    //    if (prop->IsOfObjectType(0x68686770))
    //        countHhgp++;
    //    if (prop->IsOfObjectType(0x65787070))
    //        countExpp++;
    //    if (prop->IsOfObjectType(0x65767072))
    //        countEvpr++;
    //}

    totalVehicles = countSshp + countHvrp + count6wvp;

    // Emit FontCommPop
    {
        unsigned int cmd = FontCommPop;
        unsigned char* cur = fontState.bufferCur;
        if (cur + 4 < fontState.bufferMax)
        {
            memcpy(cur, &cmd, 4);
            fontState.bufferCur += 4;
        }
    }

    // Emit FontCommDefault
    {
        unsigned int cmd = FontCommDefault;
        unsigned char* cur = fontState.bufferCur;
        if (cur + 4 < fontState.bufferMax)
        {
            memcpy(cur, &cmd, 4);
            fontState.bufferCur += 4;
        }
    }

    fontInitialiseState(&fontState);

    // Load font and emit FontCommHandle
    unsigned int font = fontFindLoadedFont("book");
    if (font != (unsigned int)-1)
    {
        unsigned char* cur = fontState.bufferCur;
        if (cur + 8 < fontState.bufferMax)
        {
            unsigned int buf[2] = { FontCommHandle, font };
            memcpy(cur, buf, 8);
            fontState.bufferCur += 8;
        }
        fontState.___u0.stack.handle = font;
        fontState.___u0.stack.header = fontArray[font].font;
    }

    // Emit FontCommFixed (scale = 10)
    {
        unsigned char* cur = fontState.bufferCur;
        if (cur + 8 < fontState.bufferMax)
        {
            unsigned int cmd = FontCommFixed;
            float         val = 10.0f;
            memcpy(cur, &cmd, 4);
            memcpy(cur + 4, &val, 4);
            fontState.bufferCur += 8;
        }
        fontState.___u0.stack.fixedw = 10.0f;
    }

    // Compute window position from font height
    float h1 = fontGetHeight();
    float h2 = fontGetHeight();
    float h3 = fontGetHeight();
    fontSetWindow(0.029999999f, h3, h2 * 0.909999967f, h1 * 2.0f + h3);

    // Emit FontCommAlign = FONT_ALIGN_RIGHT (2)
    {
        unsigned char* cur = fontState.bufferCur;
        if (cur + 8 < fontState.bufferMax)
        {
            unsigned int buf[2] = { FontCommAlign, FONT_ALIGN_RIGHT };
            memcpy(cur, buf, 8);
            fontState.bufferCur += 8;
        }
    }

    // Emit FontCommAlign = FONT_ALIGN_LEFT (0)  (reset)
    {
        unsigned char* cur = fontState.bufferCur;
        if (cur + 8 < fontState.bufferMax)
        {
            unsigned int buf[2] = { FontCommAlign, FONT_ALIGN_LEFT };
            memcpy(cur, buf, 8);
            fontState.bufferCur += 8;
        }
        fontState.___u0.stack.align = FONT_ALIGN_LEFT;
    }

    // Emit FontCommSetFlag (flag |= 8, shadow)
    {
        unsigned char* cur = fontState.bufferCur;
        if (cur + 8 < fontState.bufferMax)
        {
            unsigned int buf[2] = { FontCommSetFlag, 8 };
            memcpy(cur, buf, 8);
            fontState.bufferCur += 8;
        }
        fontState.___u0.stack.flag |= 8;
    }

    // Emit FontCommUnsetFlag (flag &= ~8)
    {
        unsigned char* cur = fontState.bufferCur;
        if (cur + 8 < fontState.bufferMax)
        {
            unsigned int buf[2] = { FontCommUnsetFlag, 8 };
            memcpy(cur, buf, 8);
            fontState.bufferCur += 8;
        }
        fontState.___u0.stack.flag &= ~8u;
    }

    // Emit FontCommPop
    {
        unsigned int cmd = FontCommPop;
        unsigned char* cur = fontState.bufferCur;
        if (cur + 4 < fontState.bufferMax)
        {
            memcpy(cur, &cmd, 4);
            fontState.bufferCur += 4;
        }
    }
}