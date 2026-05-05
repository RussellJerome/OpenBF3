// resfileparser.cpp
// Complete port of bison-generated resfileparse() from Xbox 360 XEX.
// Bison tables from YYToken.h, semantic actions ported from IDA pseudocode.


#include "framework/template/CTemplate.h"
#include "framework/template/CTemplateMgr.h"
#include "memory.h"
#include "YYToken.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ---------------------------------------------------------------------------
// Externals
// ---------------------------------------------------------------------------

int                           resfilenerrs = 0;


int yy_init_1 = 1;

_iobuf* resfileout = NULL;
char* yy_last_accepting_cpos_0 = NULL;
int resfileleng = 0;

extern char             yy_hold_char_0;
extern char* yy_c_buf_p_0;
extern int              yy_start_0;
extern yy_buffer_state* yy_current_buffer_0;
extern int              yy_n_chars_0;
extern int              yy_did_buffer_switch_on_eof_0;
extern char* resfiletext;
extern FILE* resfilein;

extern CDictionaryFileParser* s_Parse;
extern int                    g_scratchStack;
extern char* vafmtbuff(char* outBuffer, int inSize, const char* format, ...);
extern void memFreeFlags(void* mem, unsigned __int8 inFlags);
extern void* memAllocAlignCore(unsigned int length, unsigned int mgh, int align,
    const char* file, int line, const char* comment,
    char inFlags);
extern bool stringTableFind(const stringTable* table, const char* string, void** data);
extern char*                  evafmt(const char* fmt, ...);
extern void                   stricmp_wrap(const char* a, const char* b);

extern void DbgPrint(const char* format, ...);

// ---------------------------------------------------------------------------
// Token values returned by the lexer (from the asm action cases)
// ---------------------------------------------------------------------------
#define TOK_EOF         0
#define TOK_IDENTIFIER  258   // 0x102
#define TOK_STRING      259   // 0x103  (quoted string — but 273/0x111 is used!)
#define TOK_FLOAT_TOK   272   // 0x110
#define TOK_INT_TOK     271   // 0x10F
#define TOK_INCLUDE_S   273   // 0x111  string inside #include "..."
#define TOK_STRVAL      274   // 0x112  bare identifier/path used as string value

// ---------------------------------------------------------------------------
// YYSTYPE — 8-byte token value (lo=ptr/int/float, hi=string end offset)
// ---------------------------------------------------------------------------
struct YYSTYPE { int lo; int hi; };
YYSTYPE resfilelval;
static int            dword_last_accepting_state = 0;   // dword_82CCF338
void parseErr()
{
    if (!s_Parse) { DbgPrint("parseErr: s_Parse is null"); return; }

    int fidx = s_Parse->m_fileStackIndex;
    if (fidx > 0)
    {
        CDictionaryFileParser::SInputFile& fi = s_Parse->m_fileStack[fidx - 1];
        DbgPrint("Parse error in '%s' line %d", fi.m_file, fi.m_lineNo);
    }
    else
    {
        DbgPrint("Parse error in '%s'", s_Parse->m_origFile);
    }

    DbgPrint("Last token: yychar=%d resfiletext='%s' resfileleng=%d",
        resfilelval.lo, resfiletext ? resfiletext : "(null)", resfileleng);
}

//Another

// ---------------------------------------------------------------------------
// Comment skipping helper (called for /* */ style comments)
// ---------------------------------------------------------------------------
static int  yyinput_0();
static void processNewLine();
static void lexerSkipComment_0(int (*inputFunc)(), void (*newlineFunc)());

// ---------------------------------------------------------------------------
// resfile_create_buffer — allocate a new flex input buffer for a FILE*
// ---------------------------------------------------------------------------
yy_buffer_state* resfile_create_buffer(FILE* file, int size)
{
    yy_buffer_state* buf = (yy_buffer_state*)memAllocAlignCore(
        sizeof(yy_buffer_state), 0, 0,
        "resfilelexer.cpp", 0, "", 1);
    if (!buf) return nullptr;

    buf->yy_ch_buf = (char*)memAllocAlignCore(size + 2, 0, 0,
        "resfilelexer.cpp", 0, "", 1);
    if (!buf->yy_ch_buf) return nullptr;

    buf->yy_buf_size = size;
    buf->yy_input_file = file;
    buf->yy_is_our_buffer = 1;
    buf->yy_is_interactive = (file == stdin) ? 1 : 0;
    buf->yy_at_bol = 1;
    buf->yy_fill_buffer = 1;
    buf->yy_buffer_status = 0;
    buf->yy_n_chars = 0;
    buf->yy_buf_pos = buf->yy_ch_buf;

    return buf;
}

void resfilerestart(FILE* input_file)
{
    if (!yy_current_buffer_0)
        yy_current_buffer_0 = resfile_create_buffer(input_file, 0x4000);

    yy_buffer_state* buf = yy_current_buffer_0;
    buf->yy_input_file = input_file;
    buf->yy_buffer_status = 0;
    buf->yy_at_bol = 1;
    buf->yy_n_chars = 0;
    buf->yy_buf_pos = buf->yy_ch_buf;
    buf->yy_ch_buf[0] = 0;

    resfilein = input_file;
    yy_init_1 = 1;
}


// ---------------------------------------------------------------------------
// External parser state (set by CDictionaryFileParser)
// ---------------------------------------------------------------------------
extern CDictionaryFileParser* s_Parse;
static CDictionaryFileParser* s_parser = nullptr;  // local alias

// ---------------------------------------------------------------------------
// Flex DFA helper: yy_get_next_buffer_0
// Returns: 0 = more input available, 1 = EOF on current buffer,
//          2 = more input but buffer not at end, 3 = restart needed
// ---------------------------------------------------------------------------
static int yy_get_next_buffer_0()
{
    yy_buffer_state* buf = yy_current_buffer_0;

    // Save current position
    *yy_c_buf_p_0 = yy_hold_char_0;

    if (!buf->yy_buffer_status)
    {
        yy_n_chars_0 = buf->yy_n_chars;
        buf->yy_input_file = resfilein;
        buf->yy_buffer_status = 1;
    }

    // Check if we're at the end of the buffer
    if (yy_c_buf_p_0 >= &buf->yy_ch_buf[yy_n_chars_0])
    {
        // At end of buffer
        if (buf->yy_is_our_buffer && buf->yy_input_file)
        {
            // Read more input
            int num_to_read = buf->yy_buf_size - yy_n_chars_0;
            if (num_to_read > 0)
            {
                int num_read = (int)fread(&buf->yy_ch_buf[yy_n_chars_0], 1, num_to_read,
                    (FILE*)buf->yy_input_file);
                yy_n_chars_0 += num_read;
                buf->yy_n_chars = yy_n_chars_0;
                buf->yy_ch_buf[yy_n_chars_0] = 0;
                buf->yy_ch_buf[yy_n_chars_0 + 1] = 0;
                return (num_read > 0) ? 2 : 1;
            }
        }
        return 1;  // EOF
    }
    return 3;  // buffer not exhausted, restart scan
}

// ---------------------------------------------------------------------------
// yy_get_previous_state_0 — recompute DFA state from resfiletext to yy_c_buf_p_0
// ---------------------------------------------------------------------------
static int yy_get_previous_state_0()
{
    int state = yy_start_0;
    char* p = resfiletext;

    while (p < yy_c_buf_p_0)
    {
        unsigned __int8 c = (unsigned __int8)*p++;
        int ec = yy_ec[c];

        while (yy_chk[yy_base[state] + ec] != state)
        {
            state = yy_def[state];
            if (state >= 114)
                ec = yy_meta[ec];
        }
        state = yy_nxt[yy_base[state] + ec];
    }
    return state;
}

// ---------------------------------------------------------------------------
// yy_try_NUL_trans_0 — attempt DFA transition on NUL character
// ---------------------------------------------------------------------------
static int yy_try_NUL_trans_0(int current_state)
{
    int ec = yy_ec[0];
    int state = current_state;

    while (yy_chk[yy_base[state] + ec] != state)
    {
        state = yy_def[state];
        if (state >= 114)
            ec = yy_meta[ec];
    }
    int next = yy_nxt[yy_base[state] + ec];
    if (yy_base[next] == 193) return 0;  // no valid transition
    return next;
}

// ---------------------------------------------------------------------------
// yyinput_0 — read one character from the current buffer
// ---------------------------------------------------------------------------
static int yyinput_0()
{
    char c = yy_hold_char_0;

    *yy_c_buf_p_0 = yy_hold_char_0;
    ++yy_c_buf_p_0;
    yy_hold_char_0 = *yy_c_buf_p_0;
    return (unsigned __int8)c;
}

// ---------------------------------------------------------------------------
// processNewLine — increment line counter in current file stack entry
// ---------------------------------------------------------------------------
static void processNewLine()
{
    if (!s_parser) return;
    int idx = s_parser->m_fileStackIndex;
    if (idx > 0)
    {
        CDictionaryFileParser::SInputFile* fi = &s_parser->m_fileStack[idx - 1];
        fi->m_lineNo++;
    }
}

// ---------------------------------------------------------------------------
// lexerSkipComment_0 — skip /* ... */ style block comments
// ---------------------------------------------------------------------------
static void lexerSkipComment_0(int (*inputFunc)(), void (*newlineFunc)())
{
    int c;
    for (;;)
    {
        c = inputFunc();
        if (c == 0) break;  // EOF
        if (c == '\n') newlineFunc();
        if (c == '*')
        {
            c = inputFunc();
            if (c == '/') break;  // end of comment
            if (c == '\n') newlineFunc();
        }
    }
}


int resfilelex()
{
    s_parser = s_Parse;

    // Initialise on first call
    if (yy_init_1)
    {
        yy_init_1 = 0;
        if (!yy_start_0) yy_start_0 = 1;

        if (!resfilein)  resfilein = stdin;
        if (!resfileout) resfileout = stdout;

        if (!yy_current_buffer_0)
        {
            yy_current_buffer_0 = resfile_create_buffer(resfilein, 0x4000);
        }

        yy_n_chars_0 = yy_current_buffer_0->yy_n_chars;
        yy_c_buf_p_0 = yy_current_buffer_0->yy_buf_pos;
        resfilein = (FILE*)yy_current_buffer_0->yy_input_file;
        yy_hold_char_0 = *yy_c_buf_p_0;
    }

restart:
    {
        DbgPrint("RESTART: yy_c_buf_p_0 char='%c' (0x%02X)",
            *yy_c_buf_p_0 ? *yy_c_buf_p_0 : '0',
            (unsigned char)*yy_c_buf_p_0);
        char* yy_bp = yy_c_buf_p_0;  // begin pointer (start of token)
        *yy_c_buf_p_0 = yy_hold_char_0;

        char* yy_cp = yy_bp;         // current scan pointer
        int   yy_cur_state = yy_start_0;
        int   yy_last_acc_state = dword_last_accepting_state;
        char* yy_last_acc_pos = yy_last_accepting_cpos_0;

        // ---- DFA scan loop ----
    yy_match:
        while (true)
        {
            unsigned __int8 yy_c = (unsigned __int8)*yy_cp;
            int ec = yy_ec[yy_c];

            // Accept state bookkeeping
            if (yy_accept[yy_cur_state])
            {
                yy_last_acc_state = yy_cur_state;
                yy_last_acc_pos = yy_cp;
                dword_last_accepting_state = yy_cur_state;
                yy_last_accepting_cpos_0 = yy_cp;
            }

            // DFA transition
            while (yy_chk[yy_base[yy_cur_state] + ec] != yy_cur_state)
            {
                int def = yy_def[yy_cur_state];
                if ((unsigned short)def >= 114)
                    ec = yy_meta[ec];
                yy_cur_state = def;
            }
            yy_cur_state = yy_nxt[yy_base[yy_cur_state] + ec];
            ++yy_cp;

            // Check if we've hit an accepting sink state
            if ((unsigned short)yy_base[yy_cur_state] == 193)
            {
                yy_last_accepting_cpos_0 = yy_last_acc_pos;
                goto yy_find_action;
            }
        }

    yy_find_action:
        {
            int yy_act = yy_accept[yy_cur_state];
            if (!yy_act)
            {
                yy_cp = yy_last_accepting_cpos_0;
                yy_cur_state = yy_last_acc_state;
                yy_act = yy_accept[yy_cur_state];
            }

            // Set up resfiletext and resfileleng
            char yy_c_char = *yy_cp;
            char* yy_tok_start = yy_bp;
            yy_c_buf_p_0 = yy_cp;
            *yy_cp = 0;           // null-terminate token
            yy_hold_char_0 = yy_c_char;
            resfiletext = yy_tok_start;
            resfileleng = (int)(yy_cp - yy_tok_start);

            // ---- Action dispatch ----
        do_action:
            switch (yy_act)
            {
            case 0:
                // Back up: try last accepting state
                *yy_cp = yy_hold_char_0;
                yy_cp = yy_last_accepting_cpos_0;
                yy_cur_state = dword_last_accepting_state;
                goto yy_find_action;

            case 1: case 4: case 11: case 12: case 13: case 32:
                // Whitespace / ignored tokens — restart scan
                goto restart;

            case 2:
                // Newline inside file — increment line counter
            {
                int fidx = s_parser->m_fileStackIndex;
                if (fidx > 0)
                {
                    CDictionaryFileParser::SInputFile* fi =
                        &s_parser->m_fileStack[fidx - 1];
                    fi->m_lineNo++;
                }
            }
            goto restart;

            case 3:
                // Quoted string: "content"
                // resfiletext points to opening quote, resfileleng includes both quotes
                // Return pointer to first char after opening quote, length excludes both quotes
                resfilelval.lo = (int)(intptr_t)(resfiletext + 1);
                resfilelval.hi = resfileleng - 2;
                return 273;  // TOK_INCLUDE_STRING (used for both #include and regular strings)

            case 5:
                // Block comment /* ... */
                lexerSkipComment_0(yyinput_0, processNewLine);
                goto restart_buf;

            case 6: case 7: case 8:
                // Float literal (decimal)
            {
                float f = (float)strtod(resfiletext, nullptr);
                memcpy(&resfilelval.lo, &f, 4);
                resfilelval.hi = 0;
            }
            return 272;  // TOK_FLOAT

            case 9:
                // Integer literal (decimal)
                resfilelval.lo = (int)atol(resfiletext);
                resfilelval.hi = 0;
                return 271;  // TOK_INT

            case 10:
                // Integer literal (hex)
                resfilelval.lo = (int)strtol(resfiletext, nullptr, 16);
                resfilelval.hi = 0;
                return 271;  // TOK_INT

            case 14: return 258;
            case 15: return 259;
            case 16: return 261;
            case 17: return 262;
            case 18: return 260;
            case 19: return 263;
            case 20: return 269;
            case 21: return 268;
            case 22: return 264;
            case 23: case 24: return 265;
            case 25: return 266;
            case 26: case 27: return 267;
            case 28: return 270;

            case 29:
                // Float literal: FCNULL (3.4028e38)
            {
                float f = 3.4027999e38f;
                memcpy(&resfilelval.lo, &f, 4);
                resfilelval.hi = 0;
            }
            return 272;

            case 30:
                // Bare identifier used as string value (full token)
                resfilelval.lo = (int)(intptr_t)resfiletext;
                resfilelval.hi = resfileleng;
                return 274;

            case 31:
                // Dotted identifier (e.g. path/to/file) — return up to first '.'
            {
                resfilelval.lo = (int)(intptr_t)resfiletext;
                resfilelval.hi = resfileleng;
                // Find the dot to truncate at
                char* p = resfiletext;
                for (unsigned int j = 0; j < (unsigned int)resfileleng; j++, p++)
                {
                    if (*p == '.') { resfilelval.hi = (int)j; break; }
                }
            }
            return 274;

            case 33:
                // Echo token to output (used in some flex modes)
                fwrite(resfiletext, resfileleng, 1, resfileout);
                goto restart_buf;

            case 34:
            {
                int save_len = (int)(yy_cp - resfiletext) - 1;
                *yy_cp = yy_hold_char_0;
                yy_buffer_state* buf = yy_current_buffer_0;
                if (!buf->yy_buffer_status)
                {
                    yy_n_chars_0 = buf->yy_n_chars;
                    buf->yy_input_file = resfilein;
                    buf->yy_buffer_status = 1;
                }
                if (yy_c_buf_p_0 <= &buf->yy_ch_buf[yy_n_chars_0])
                {
                    // Mid-buffer NUL — parser's null-term trick, not real end of buffer.
                    // Just accept whatever token was last matched.
                    yy_c_buf_p_0 = &resfiletext[save_len];
                    yy_cur_state = yy_last_acc_state;
                    yy_tok_start = resfiletext;
                    goto yy_find_action;
                }
                // Need more input
                unsigned int next = yy_get_next_buffer_0();
                if (next == 0)
                {
                    resfiletext = yy_tok_start;
                    yy_cp = &resfiletext[save_len];
                    yy_c_buf_p_0 = yy_cp;
                    int prev = yy_get_previous_state_0();
                    buf = yy_current_buffer_0;
                    yy_cur_state = prev;
                    yy_tok_start = resfiletext;
                    goto yy_match;
                }
                else if (next == 1)
                {
                    yy_did_buffer_switch_on_eof_0 = 0;
                    if (s_parser->PopInputFile())
                    {
                        if (!yy_did_buffer_switch_on_eof_0)
                            resfilerestart(resfilein);
                        goto restart_buf;
                    }
                    else
                    {
                        resfiletext = yy_tok_start;
                        buf = yy_current_buffer_0;
                        yy_c_buf_p_0 = resfiletext;
                        yy_act = (yy_start_0 - 1) / 2 + 35;
                        goto do_action;
                    }
                }
                else if (next >= 3)
                {
                    goto restart_buf;
                }
                else
                {
                    buf = yy_current_buffer_0;
                    yy_cp = &buf->yy_ch_buf[yy_n_chars_0];
                    yy_c_buf_p_0 = yy_cp;
                    int prev = yy_get_previous_state_0();
                    yy_tok_start = resfiletext;
                    yy_cur_state = prev;
                    goto yy_find_action;
                }
            }

            case 35:
                // EOF
                return 0;

            default:
                fprintf(stderr, "fatal flex scanner internal error--no action found\n");
                exit(2);
            }

        restart_buf:
            yy_bp = yy_c_buf_p_0;
            goto restart;
        }
    }
}

// ---------------------------------------------------------------------------
// Stack slot layout (16 bytes per entry in m_Stack[256])
// ---------------------------------------------------------------------------
struct StackSlot
{
    unsigned int  flags;   // +0
    CTemplate*    tmpl;    // +4
    CDictionary*  dict;    // +8  (also cast to CArray*)
    CDictionary*  src;     // +12
};
static StackSlot* slot(CDictionaryFileParser* p, int i)
{
    return (StackSlot*)(p->m_Stack + i * 16);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static char nullTerm(char* s, int off)
{
    if (!s || off < 0) return 0;
    char c = s[off]; s[off] = 0; return c;
}
static void restore(char* s, int off, char c)
{
    if (s && off >= 0) s[off] = c;
}
static CTemplate* findTmpl(const char* name)
{
    if (!name || !*name) return nullptr;
    void* r = nullptr;
    stringTableFind(&CTemplateMgr::s_mgr->m_hash.m_StringTable, name, &r);
    return (CTemplate*)r;
}

// ---------------------------------------------------------------------------
// Parser stack value type (8 bytes, matching 64-bit ld/std in asm)
// ---------------------------------------------------------------------------
struct YYVal { int lo; int hi; };

// ---------------------------------------------------------------------------
// resfileparse() — standard bison LALR(1) machine
// ---------------------------------------------------------------------------
int resfileparse()
{
    static const int INIT = 200, MAX = 10000;

    short  yyssa[INIT]; YYVal yyvsa[INIT];
    short* yyss = yyssa; YYVal* yyvs = yyvsa;
    int    yystacksize = INIT;
    short* yyssp = yyss; YYVal* yyvsp = yyvs;

    int yystate = 0, yychar = -2, yynerrs = 0, yyerrstatus = 0;
    resfilenerrs = 0;
    resfilelval.lo = -2;

    CDictionaryFileParser* P = s_Parse;
    const char* DEF = "default";

    *yyssp = 0; yyvsp->lo = 0; yyvsp->hi = 0;

    // ------------------------------------------------------------------
    // REDUCE macro — shared between action-table and yydefault paths
    // ------------------------------------------------------------------
#define DO_REDUCE(rule_) \
    do { \
        int    _r   = (rule_); \
        int    _rhs = (int)(unsigned char)yyr2_1[_r]; \
        YYVal* _lhs = yyvsp - _rhs + 1; /* $1 */ \
        YYVal  _res; _res.lo = 0; _res.hi = 0; \
        DbgPrint("DO_REDUCE: rule=%d rhs=%d yystate=%d ssp_offset=%d",\
            _r, (int)(unsigned char)yyr2_1[_r], yystate, (int)(yyssp - yyss));\
        DbgPrint("REDUCE rule=%d lhs=%d rhs=%d", _r, (int)(unsigned char)yyr1_1[_r], _rhs); \
        switch (_r) { \
            case 3: case 8: \
                _res.lo = 0; _res.hi = 0; break; \
            case 4: case 9: \
                _res = *yyvsp; break; \
            case 10: { \
                /* assignment: name offset '{' body '}'  -- template dict block */ \
                char* nm  = (char*)(intptr_t)_lhs[0].lo; \
                int   nmo = _lhs[0].hi; \
                char  sv  = nullTerm(nm, nmo); \
                CDictionary* dict = P->PushTemplate(nm); \
                P = s_Parse; \
                /* _lhs[1].lo = type string ptr, _lhs[1].hi = type offset */ \
                int   tlo = _lhs[1].lo; \
                if (tlo) { \
                    char* ts  = (char*)(intptr_t)tlo; \
                    int   tso = _lhs[1].hi; \
                    char  sv2 = nullTerm(ts, tso); \
                    CTemplate* tmpl = nullptr; \
                    if (ts && *ts) tmpl = findTmpl(ts); \
                    if (dict && tmpl) dict->SetTemplate(tmpl); \
                    if (tmpl && tmpl->m_isFieldMeta && dict) { \
                        P = s_Parse; \
                        int si = P->m_StackIndex * 16; \
                        CDictionary* cur = *(CDictionary**)((char*)&P->m_flags + si + 4); \
                        if (cur) ((unsigned __int8*)cur)[16] = 1; \
                    } \
                    restore(ts, tso, sv2); \
                } \
                restore(nm, nmo, sv); \
                break; \
            } \
            case 11: { \
                /* dict_block: name offset value offset */ \
                char* nm  = (char*)(intptr_t)_lhs[0].lo; \
                int   nmo = _lhs[0].hi; \
                char* vl  = (char*)(intptr_t)yyvsp[0].lo; \
                int   vlo = yyvsp[0].hi; \
                char sv1  = nullTerm(nm, nmo); \
                char sv2  = nullTerm(vl, vlo); \
                /* Look up template for value type name */ \
                CTemplate* valTmpl = nullptr; \
                if (vl && *vl) valTmpl = findTmpl(vl); \
                if (!valTmpl) { \
                    /* Check if current stack template has a field meta */ \
                    int si = P->m_StackIndex; \
                    StackSlot* ss2 = slot(P, si); \
                    CTemplate* curTmpl = ss2->tmpl; \
                    if (curTmpl && curTmpl->m_isFieldMeta \
                        && _stricmp(vl ? vl : "", DEF) == 0 \
                        && !(P->m_flags & 2)) \
                    { \
                        CTemplate* prev = P->PrevTemplate(); \
                        if (prev) { \
                            const char* mfn = curTmpl->GetMetaDataFieldName(); \
                            char* fmt = evafmt("%s.%s", mfn, nm); \
                            CDictionary* d2 = P->PushTemplate(fmt); \
                            P = s_Parse; \
                            if (d2) d2->SetTemplate(valTmpl); \
                        } \
                    } else { \
                        char* tmplFmt = evafmt("%s.%s", \
                            (curTmpl && curTmpl->ele.string) ? curTmpl->ele.string : "", nm); \
                        CDictionary* d2 = P->PushTemplate(tmplFmt); \
                        P = s_Parse; \
                        if (d2) d2->SetTemplate(valTmpl); \
                    } \
                } else { \
                    if (valTmpl->m_isFieldMeta) { \
                        int si = P->m_StackIndex; \
                        StackSlot* ss2 = slot(P, si); \
                        CTemplate* curTmpl = ss2->tmpl; \
                        if (curTmpl && !curTmpl->m_isFieldMeta) { \
                            char* fmt = evafmt(".%s.%s", \
                                curTmpl->ele.string ? curTmpl->ele.string : "", nm); \
                            CDictionary* d2 = P->PushTemplate(fmt); \
                            P = s_Parse; \
                            if (d2) d2->SetTemplate(valTmpl); \
                            int si2 = P->m_StackIndex * 16; \
                            CDictionary* cur2 = *(CDictionary**)((char*)&P->m_flags + si2 + 4); \
                            if (cur2) ((unsigned __int8*)cur2)[16] = 1; \
                        } \
                    } \
                } \
                restore(vl, vlo, sv2); restore(nm, nmo, sv1); \
                break; \
            } \
            case 12: _res.hi = 1 << 24; break;  /* prim_type STRING */ \
            case 13: _res.hi = 2 << 24; break;  /* prim_type INT    */ \
            case 14: _res.hi = 4 << 24; break;  /* prim_type DICT   */ \
            case 15: { \
                /* array_def: prim_type template key */ \
                unsigned __int8 pt  = (unsigned __int8)((_lhs[0].hi >> 24) & 0xFF); \
                CTemplate*      tm  = (CTemplate*)(intptr_t)_lhs[1].lo; \
                char* key = (char*)(intptr_t)_lhs[2].lo; \
                int   koff = _lhs[2].hi; \
                char sv   = nullTerm(key, koff); \
                P->PushArray(pt, tm, key); \
                restore(key, koff, sv); \
                P = s_Parse; \
                break; \
            } \
            case 16: { \
                /* array_def: auto-typed from parent template */ \
                char* key = (char*)(intptr_t)_lhs[0].lo; \
                int   koff = _lhs[0].hi; \
                char sv   = nullTerm(key, koff); \
                unsigned __int8 primType = 0; CTemplate* tmpl = nullptr; \
                int parentLevel = (P->m_StackIndex + 2) * 16; \
                if (parentLevel < 256) { \
                    CDictionary* pdict = *(CDictionary**)((char*)&P->m_flags + parentLevel); \
                    if (pdict) { \
                        CTemplate* pt2 = pdict->GetTemplate(); \
                        if (pt2) { \
                            unsigned __int8 vt = 0; TDictDataValue vv = {}; \
                            if (pt2->m_dict && pt2->m_dict->GetOptionalValue(key, &vt, &vv, nullptr) \
                                && vt == 16 && vv.d) \
                                ((CArray*)vv.d)->GetArrayType(&primType, &tmpl); \
                        } \
                    } \
                } \
                P = s_Parse; \
                P->PushArray(primType, tmpl, key); \
                restore(key, koff, sv); \
                P = s_Parse; \
                break; \
            } \
            case 17: { \
                /* array_def: dict-typed key */ \
                char* tnm  = (char*)(intptr_t)_lhs[0].lo; int tnmo = _lhs[0].hi; \
                char* key  = (char*)(intptr_t)_lhs[2].lo; int  koff = _lhs[2].hi; \
                char sv1   = nullTerm(tnm, tnmo); \
                CTemplate* tm = findTmpl(tnm); \
                restore(tnm, tnmo, sv1); \
                char sv2   = nullTerm(key, koff); \
                P->PushArray(8, tm, key); \
                restore(key, koff, sv2); \
                P = s_Parse; \
                break; \
            } \
            case 20: \
                if (P->m_StackIndex > 0) P->m_StackIndex--; \
                break; \
            case 24: { \
                /* string_value / value pass-through with offset */ \
                _res = *yyvsp; break; \
            } \
            case 27: { \
                /* string item in array */ \
                char* val = (char*)(intptr_t)yyvsp[0].lo; int voff = yyvsp[0].hi; \
                char sv   = nullTerm(val, voff); \
                int si    = P->m_StackIndex * 16; \
                CArray* arr = *(CArray**)((char*)&P->m_flags + si + 8); \
                if (arr) { \
                    unsigned __int8 at = 0; CTemplate* atm = nullptr; \
                    arr->GetArrayType(&at, &atm); \
                    if (!at || at == 4) { \
                        const char* cp = s_Parse->CopyValueString(val); \
                        if (!((unsigned __int8*)arr)[4]) ((unsigned __int8*)arr)[4] = 4; \
                        TDictDataValue dv = {}; dv.s = cp; \
                        arr->AppendItem(&dv, 0); \
                    } \
                } \
                restore(val, voff, sv); \
                P = s_Parse; \
                break; \
            } \
            case 28: { \
                /* int item in array */ \
                int si = P->m_StackIndex * 16; \
                CArray* arr = *(CArray**)((char*)&P->m_flags + si + 8); \
                if (arr) { \
                    unsigned __int8 at = 0; CTemplate* atm = nullptr; \
                    arr->GetArrayType(&at, &atm); \
                    if (!at || at == 1) { \
                        if (!((unsigned __int8*)arr)[4]) ((unsigned __int8*)arr)[4] = 1; \
                        TDictDataValue dv = {}; dv.i = yyvsp[0].lo; \
                        arr->AppendItem(&dv, 0); \
                    } \
                } \
                P = s_Parse; \
                break; \
            } \
            case 29: { \
                /* float item in array */ \
                int si = P->m_StackIndex * 16; \
                CArray* arr = *(CArray**)((char*)&P->m_flags + si + 8); \
                if (arr) { \
                    unsigned __int8 at = 0; CTemplate* atm = nullptr; \
                    arr->GetArrayType(&at, &atm); \
                    if (!at || at == 2) { \
                        if (!((unsigned __int8*)arr)[4]) ((unsigned __int8*)arr)[4] = 2; \
                        float f; memcpy(&f, &yyvsp[0].lo, 4); \
                        TDictDataValue dv = {}; dv.f = f; \
                        arr->AppendItem(&dv, 0); \
                    } \
                } \
                P = s_Parse; \
                break; \
            } \
            case 31: { \
            DbgPrint("rule31 entry: m_StackIndex=%d", P->m_StackIndex); \
               DbgPrint("rule31: stack+0=%p +4=%p +8=%p +12=%p", \
                    *(void**)((char*)P->m_Stack + P->m_StackIndex*16 + 0), \
                    *(void**)((char*)P->m_Stack + P->m_StackIndex*16 + 4), \
                    *(void**)((char*)P->m_Stack + P->m_StackIndex*16 + 8), \
                    *(void**)((char*)P->m_Stack + P->m_StackIndex*16 + 12)); \
                /* dict item in array - @TypeName { body } */ \
                char* tnm = (char*)(intptr_t)yyvsp[0].lo; int toff = yyvsp[0].hi; \
                char sv   = nullTerm(tnm, toff); \
                /* Compare with "@" to detect @-prefixed type */ \
                bool isAt = (tnm && tnm[0] == '@'); \
                CTemplate* tmpl = nullptr; \
                if (!isAt && tnm && *tnm) tmpl = findTmpl(tnm); \
                int si    = P->m_StackIndex * 16; \
                DbgPrint("rule31: m_StackIndex=%d si=%d arr_ptr=%p", P->m_StackIndex, si, (char*)&P->m_flags + si + 8);\
                CArray* arr = *(CArray**)((char*)P->m_Stack + si + 12); \
                DbgPrint("rule31: arr=%p", arr);\
                restore(tnm, toff, sv); \
                if (arr) { \
                    CDictionary* dict = P->PushDict(false); \
                    P = s_Parse; \
                    if (dict) { \
                        if (tmpl) dict->SetTemplate(tmpl); \
                        if (!((unsigned __int8*)arr)[4]) ((unsigned __int8*)arr)[4] = 8; \
                        TDictDataValue dv = {}; dv.d = dict; \
                        arr->AppendItem(&dv, 0); \
                    } \
                } \
                break; \
            } \
            case 32: case 35: \
                if (P->m_StackIndex > 0) P->m_StackIndex--; \
                break; \
            case 33: case 34: \
                _res = *yyvsp; break; \
            case 36: { \
                char* fn  = (char*)(intptr_t)yyvsp[0].lo; \
                int   fno = yyvsp[0].hi; \
                char sv   = nullTerm(fn, fno); \
                if (P->m_includeListStrs && P->m_includeList) { \
                    const char* cp = P->m_includeListStrs->Copy(fn); \
                    (void)cp; \
                } \
                P->PushInputFile(fn); \
                restore(fn, fno, sv); \
                P = s_Parse; \
                yychar = -2; \
                break; \
            } \
            case 39: { \
                /* key = "string" -- simple string assignment */ \
                char* key = (char*)(intptr_t)_lhs[0].lo; int  koff = _lhs[0].hi; \
                char* val = (char*)(intptr_t)yyvsp[0].lo; int  voff = yyvsp[0].hi; \
                char svk  = nullTerm(key, koff); char svv = nullTerm(val, voff); \
                const char* cp = s_Parse->CopyValueString(val); \
                TDictDataValue dv = {}; dv.s = cp; \
                s_Parse->EnterValue(key, 4, &dv); \
                restore(val, voff, svv); restore(key, koff, svk); \
                P = s_Parse; \
                break; \
            } \
            case 40: { \
                /* key = "existing"|"new"  -- look up existing value and prepend */ \
                char* key  = (char*)(intptr_t)_lhs[0].lo; int  koff = _lhs[0].hi; \
                char* newv = (char*)(intptr_t)yyvsp[0].lo; int  noff = yyvsp[0].hi; \
                char svk   = nullTerm(key, koff); char svn = nullTerm(newv, noff); \
                const char* existVal = nullptr; \
                { \
                    int si = P->m_StackIndex * 16; \
                    CDictionary* cd = *(CDictionary**)((char*)&P->m_flags + si); \
                    if (cd) { CTemplate* t2 = cd->GetTemplate(); \
                        if (t2 && t2->m_dict) { \
                            unsigned __int8 vt = 0; TDictDataValue vv = {}; \
                            if (t2->m_dict->GetOptionalValue(key, &vt, &vv, nullptr) && vt == 4 && vv.s) \
                                existVal = vv.s; \
                        } \
                    } \
                } \
                const char* cp; \
                if (existVal) { \
                    int t = (int)(strlen(existVal) + strlen(newv) + 2); \
                    char* buf = t > 0 ? (char*)memAllocAlignCore(t, g_scratchStack, 0, \
                        "source/dictionary/resfileparser.y", 598, "", 1) : nullptr; \
                    if (buf) { vafmtbuff(buf, t, "%s%s", existVal, newv); \
                        cp = s_Parse->CopyValueString(buf); memFreeFlags(buf, 1); } \
                    else cp = s_Parse->CopyValueString(newv); \
                } else cp = s_Parse->CopyValueString(newv); \
                TDictDataValue dv = {}; dv.s = cp; \
                s_Parse->EnterValue(key, 4, &dv); \
                restore(newv, noff, svn); restore(key, koff, svk); \
                P = s_Parse; \
                break; \
            } \
            case 41: { \
                /* key = "a" | "b"  -- pipe-merge two string tokens */ \
                char* key  = (char*)(intptr_t)_lhs[0].lo; int  koff = _lhs[0].hi; \
                char* va   = (char*)(intptr_t)_lhs[2].lo; int  aoff = _lhs[2].hi; \
                char* vb   = (char*)(intptr_t)yyvsp[0].lo; int  boff = yyvsp[0].hi; \
                char svk   = nullTerm(key, koff); \
                char sva   = nullTerm(va,  aoff); \
                char svb   = nullTerm(vb,  boff); \
                const char* existVal = nullptr; \
                { \
                    int si = P->m_StackIndex * 16; \
                    CDictionary* cd = *(CDictionary**)((char*)&P->m_flags + si); \
                    if (cd) { CTemplate* t2 = cd->GetTemplate(); \
                        if (t2 && t2->m_dict) { \
                            unsigned __int8 vt = 0; TDictDataValue vv = {}; \
                            if (t2->m_dict->GetOptionalValue(key, &vt, &vv, nullptr) && vt == 4 && vv.s) \
                                existVal = vv.s; \
                        } \
                    } \
                } \
                const char* cp; \
                if (existVal) { \
                    int lenA = (int)strlen(va); int lenB = (int)strlen(existVal); \
                    const char* sep = (lenA > 0 && lenB > 0) ? "|" : ""; \
                    int t = lenA + lenB + 2; \
                    char* buf = t > 0 ? (char*)memAllocAlignCore(t, g_scratchStack, 0, \
                        "source/dictionary/resfileparser.y", 636, "", 1) : nullptr; \
                    if (buf) { vafmtbuff(buf, t, "%s%s%s", existVal, sep, va); \
                        cp = s_Parse->CopyValueString(buf); memFreeFlags(buf, 1); } \
                    else cp = s_Parse->CopyValueString(va); \
                } else cp = s_Parse->CopyValueString(va); \
                TDictDataValue dv = {}; dv.s = cp; \
                s_Parse->EnterValue(key, 4, &dv); \
                restore(vb, boff, svb); restore(va, aoff, sva); restore(key, koff, svk); \
                P = s_Parse; \
                break; \
            } \
            case 42: { \
                /* key = integer */ \
                char* key = (char*)(intptr_t)_lhs[0].lo; int koff = _lhs[0].hi; \
                char sv   = nullTerm(key, koff); \
                TDictDataValue dv = {}; dv.i = yyvsp[0].lo; \
                P->EnterValue(key, 1, &dv); \
                restore(key, koff, sv); P = s_Parse; \
                break; \
            } \
            case 43: { \
                /* key = float */ \
                char* key = (char*)(intptr_t)_lhs[0].lo; int koff = _lhs[0].hi; \
                char sv   = nullTerm(key, koff); \
                float f; memcpy(&f, &yyvsp[0].lo, 4); \
                TDictDataValue dv = {}; dv.f = f; \
                P->EnterValue(key, 2, &dv); \
                restore(key, koff, sv); P = s_Parse; \
                break; \
            } \
            default: break; \
        } \
        /* Pop RHS, push result */ \
        yyssp -= _rhs; yyvsp -= _rhs; \
        ++yyssp; ++yyvsp; \
        yyvsp->lo = _res.lo; yyvsp->hi = _res.hi; \
        /* Goto new state */ \
        { \
            int _lsym  = (int)(unsigned char)yyr1_1[_r]; \
            int _cur   = (int)(short)*yyssp; \
            int _pg    = (signed char)yypgoto_1[_lsym - 20]; \
            int _ci    = _pg + _cur; \
            int _ns; \
            if ((unsigned int)_ci <= 57 && (int)(unsigned char)yycheck_1[_ci] == _cur) \
                _ns = (signed char)yytable_1[_ci]; \
            else \
                _ns = (signed char)yydefgoto_1[_lsym - 20]; \
            yystate = _ns; \
        } \
    } while (0)

    // ------------------------------------------------------------------
    // Main parse loop
    // ------------------------------------------------------------------
    for (;;)
    {
        // Grow stack if needed
        if (yyssp >= yyss + yystacksize - 1)
        {
            int yysize = (int)(yyssp - yyss) + 1;
            if (yystacksize >= MAX) goto yyoverflowlab;
            int newsize = yystacksize * 2; if (newsize > MAX) newsize = MAX;
            int ssb = newsize * (int)sizeof(short);
            int vsb = newsize * (int)sizeof(YYVal);
            char* buf = (char*)malloc(ssb + vsb + 8);
            if (!buf) goto yyoverflowlab;
            short* nss = (short*)buf;
            YYVal* nvs = (YYVal*)(buf + ((ssb + 7) & ~7));
            memcpy(nss, yyss, yysize * sizeof(short));
            memcpy(nvs, yyvs, yysize * sizeof(YYVal));
            if (yyss != yyssa) free(yyss);
            yyss = nss; yyvs = nvs;
            yyssp = yyss + yysize - 1; yyvsp = yyvs + yysize - 1;
            yystacksize = newsize;
        }

        *yyssp = (short)yystate;

        {
            int yyn = (signed char)yypact_1[yystate];
            DbgPrint("STATE=%d yypact=%d yychar=%d", yystate, yyn, yychar);
            if (yyn == -16) goto yydefault;

            if (yychar == -2)
            {
                yychar = resfilelex();
                P = s_Parse;
                DbgPrint("FETCH: token=%d text='%s'", yychar, resfiletext ? resfiletext : "(null)");
            }
            else
            {
                DbgPrint("REUSE: yychar=%d", yychar);
            }

            DbgPrint("LEX -> token=%d text='%s' lo=%d hi=%d", yychar,
                resfiletext ? resfiletext : "(null)", resfilelval.lo, resfilelval.hi);

            int yyx;
            if      (yychar <= 0)        { yyx = 0; yychar = 0; }
            else if (yychar > 0x112)     { yyx = 2; }
            else                         { yyx = (int)(unsigned char)yytranslate_1[yychar]; }

            //test replacement
            int idx = yyn + yyx;
            if ((unsigned int)idx > 57 || (int)(unsigned char)yycheck_1[idx] != yyx)
            {
                if (yystate == 42 && yychar == 0)
                    goto yyacceptlab;
                goto yydefault;
            }

            int yyact = (signed char)yytable_1[idx];
            if (yyact > 0)
            {
                if (yyact == 3) goto yyacceptlab;
                if (yychar) yychar = -2;
                ++yyssp; ++yyvsp;
                yyvsp->lo = resfilelval.lo; yyvsp->hi = resfilelval.hi;
                if (yyerrstatus) yyerrstatus--;
                yystate = yyact;
                continue;
            }
            else
            {
                if (yyact == 0 || yyact == -24) goto yyerrlab;
                DO_REDUCE(-yyact);
                continue;
            }
        }

yydefault:
        {
            int rule = (int)(unsigned char)yydefact_1[yystate];
            if (!rule) goto yyerrlab;
            DO_REDUCE(rule);
            continue;
        }

yyerrlab:
        if (!yyerrstatus)
        {
            ++resfilenerrs; ++yynerrs;
            if (P) { parseErr(); P = s_Parse; }
        }
        if (yyerrstatus == 3) { if (!yychar) goto yyabortlab; yychar = -2; }
        yyerrstatus = 3;

        for (;;)
        {
            int yyn = (signed char)yypact_1[yystate];
            if (yyn != -16)
            {
                int idx = yyn + 1;
                if ((unsigned int)idx <= 57 && (int)(unsigned char)yycheck_1[idx] == 1)
                {
                    int yyact = (signed char)yytable_1[idx];
                    if (yyact > 0)
                    {
                        if (yyact == 3) goto yyacceptlab;
                        ++yyssp; ++yyvsp;
                        yyvsp->lo = resfilelval.lo; yyvsp->hi = resfilelval.hi;
                        yystate = yyact;
                        break;
                    }
                }
            }
            if (yyssp == yyss) goto yyabortlab;
            --yyssp; --yyvsp;
            yystate = *yyssp;
        }
        continue;
    }

yyacceptlab: if (yyss != yyssa) free(yyss); return 0;
yyabortlab:  if (yyss != yyssa) free(yyss); return 1;
yyoverflowlab: if (P) parseErr(); if (yyss != yyssa) free(yyss); return 2;

#undef DO_REDUCE
}
