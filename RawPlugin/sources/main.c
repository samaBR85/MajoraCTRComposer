#include <3ds.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "plgldr.h"
#include "csvc.h"
#include "common.h"
#include "font6x10.h"
#include "sysfont.h"
#include "glyphs.h"
#include "guide.h"
#include "themes.h"
#include "sprites.h"
#include "topbg.h"
#include "botbg.h"
#include "logo.h"

#include "plugin/identity.inc.c"

#include "engine/platform.inc.c"

// ===================== RAM compose buffer (RGB888, row-major, heap) =====================
static u8  *gCompose;  // TOP_W * TOP_H * 3
static u16 *savedBot;  // BOT_W * BOT_H (RGB565 backup of the game's bottom frame)
static u16 *savedTop;  // TOP_W * TOP_H (RGB565 backup of the dimmed top backdrop)
static int  savedTopValid;

static inline u8 *CPix(int x, int y) { return &gCompose[(y * TOP_W + x) * 3]; }

static void CFill(int x0, int y0, int w, int h, u8 r, u8 g, u8 b)
{
    if (x0 < 0) { w += x0; x0 = 0; }
    if (y0 < 0) { h += y0; y0 = 0; }
    if (x0 + w > TOP_W) w = TOP_W - x0;
    if (y0 + h > TOP_H) h = TOP_H - y0;
    for (int y = y0; y < y0 + h; ++y)
    {
        u8 *p = CPix(x0, y);
        for (int x = 0; x < w; ++x, p += 3) { p[0] = r; p[1] = g; p[2] = b; }
    }
}

static void CFillBlend(int x0, int y0, int w, int h, u8 r, u8 g, u8 b, u8 a)
{
    if (x0 < 0) { w += x0; x0 = 0; }
    if (y0 < 0) { h += y0; y0 = 0; }
    if (x0 + w > TOP_W) w = TOP_W - x0;
    if (y0 + h > TOP_H) h = TOP_H - y0;
    for (int y = y0; y < y0 + h; ++y)
    {
        u8 *p = CPix(x0, y);
        for (int x = 0; x < w; ++x, p += 3)
        {
            p[0] = (u8)((p[0] * (255 - a) + r * a) / 255);
            p[1] = (u8)((p[1] * (255 - a) + g * a) / 255);
            p[2] = (u8)((p[2] * (255 - a) + b * a) / 255);
        }
    }
}

// The 6x10 font is ASCII-only, so the small font strips accents to the base
// letter (é->e, ç->c). Only the Cheat Search form / hints / footers use it.
static unsigned char SmallAscii(u32 code)
{
    if (code < 0x80) return (unsigned char)code;
    switch (code)
    {
        case 0x00C0: case 0x00C1: case 0x00C2: case 0x00C3: case 0x00C4: case 0x00C5: return 'A';
        case 0x00C7: return 'C';
        case 0x00C8: case 0x00C9: case 0x00CA: case 0x00CB: return 'E';
        case 0x00CC: case 0x00CD: case 0x00CE: case 0x00CF: return 'I';
        case 0x00D1: return 'N';
        case 0x00D2: case 0x00D3: case 0x00D4: case 0x00D5: case 0x00D6: return 'O';
        case 0x00D9: case 0x00DA: case 0x00DB: case 0x00DC: return 'U';
        case 0x00DF: return 's';
        case 0x00E0: case 0x00E1: case 0x00E2: case 0x00E3: case 0x00E4: case 0x00E5: return 'a';
        case 0x00E7: return 'c';
        case 0x00E8: case 0x00E9: case 0x00EA: case 0x00EB: return 'e';
        case 0x00EC: case 0x00ED: case 0x00EE: case 0x00EF: return 'i';
        case 0x00F1: return 'n';
        case 0x00F2: case 0x00F3: case 0x00F4: case 0x00F5: case 0x00F6: return 'o';
        case 0x00F9: case 0x00FA: case 0x00FB: case 0x00FC: return 'u';
        case 0x00AA: return 'a';   case 0x00BA: return 'o';
        case 0x00A1: return '!';   case 0x00BF: return '?';
        case 0x00AB: return '<';   case 0x00BB: return '>';
        default: return 0;         // undrawable: skip
    }
}

static void CText6(int x, int y, const char *s, u8 r, u8 g, u8 b)
{
    while (*s)
    {
        unsigned char ch = SmallAscii(SysFontUtf8Next(&s));
        if (!ch) continue;
        const unsigned char *glyph = &font[ch * FONT_HEIGHT];
        for (int dy = 0; dy < FONT_HEIGHT; ++dy)
            for (int dx = 0; dx < FONT_WIDTH; ++dx)
                if (glyph[dy] & (0x80 >> dx))
                {
                    int X = x + dx, Y = y + dy;
                    if ((unsigned)X < TOP_W && (unsigned)Y < TOP_H)
                    { u8 *p = CPix(X, Y); p[0] = r; p[1] = g; p[2] = b; }
                }
        x += FONT_WIDTH + 1;
    }
}

// 6x10 scaled 1.5x — fallback if the system font is unavailable
static void CText15(int x, int y, const char *s, u8 r, u8 g, u8 b)
{
    for (; *s; ++s)
    {
        const unsigned char *glyph = &font[(unsigned char)*s * FONT_HEIGHT];
        for (int dy = 0; dy < 15; ++dy)
        {
            unsigned char bits = glyph[dy * 2 / 3];
            for (int dx = 0; dx < 9; ++dx)
                if (bits & (0x80 >> (dx * 2 / 3)))
                {
                    int X = x + dx, Y = y + dy;
                    if ((unsigned)X < TOP_W && (unsigned)Y < TOP_H)
                    { u8 *p = CPix(X, Y); p[0] = r; p[1] = g; p[2] = b; }
                }
        }
        x += 10;
    }
}

static void CText(int x, int y, const char *s, u8 r, u8 g, u8 b, int bold)
{
    if (SysFontReady()) SysFontDrawText(gCompose, x, y, s, r, g, b, bold);
    else                CText15(x, y + 1, s, r, g, b);
}
static int CTextWidth(const char *s)
{
    if (SysFontReady()) return SysFontTextWidth(s);
    int n = 0; while (s[n]) n++;
    return n * 10;
}

// Draw text, truncating with ".." if it would exceed maxw pixels.
static void CTextClip(int x, int y, const char *s, int maxw, u8 r, u8 g, u8 b, int bold)
{
    char buf[72];
    int n = 0;
    while (s[n] && n < 70) { buf[n] = s[n]; n++; }
    buf[n] = 0;
    if (CTextWidth(buf) <= maxw) { CText(x, y, buf, r, g, b, bold); return; }
    while (n > 1)
    {
        buf[--n] = 0;
        char tmp[74];
        int t = 0;
        for (int i = 0; i < n; ++i) tmp[t++] = buf[i];
        tmp[t++] = '.'; tmp[t++] = '.'; tmp[t] = 0;
        if (CTextWidth(tmp) <= maxw) { CText(x, y, tmp, r, g, b, bold); return; }
    }
    CText(x, y, "..", r, g, b, bold);
}

// ===================== Framebuffer <-> compose =====================
typedef struct { u32 fb; u32 stride; u32 bpp; u32 fmt; } FbInfo;

static FbInfo GetFb(int hidden)
{
    u32 fmt = REG32(LCD_TOP + LCD_FORMAT) & 7;
    u32 sel = REG32(LCD_TOP + LCD_SELECT) & 1;
    FbInfo f;
    f.fmt    = fmt;
    f.bpp    = (fmt == 0) ? 4u : (fmt == 1) ? 3u : 2u;
    f.stride = REG32(LCD_TOP + LCD_STRIDE);
    if (hidden) f.fb = REG32(LCD_TOP + (sel ? LCD_FBA1 : LCD_FBA2));
    else        f.fb = REG32(LCD_TOP + (sel ? LCD_FBA2 : LCD_FBA1));
    return f;
}

static inline void FbWritePx(const FbInfo *f, int x, int y, const u8 *p, int screenH)
{
    volatile u8 *px = (volatile u8 *)((f->fb + (u32)x * f->stride + (u32)(screenH - 1 - y) * f->bpp) | (1u << 31));
    if (f->bpp >= 3) { px[0] = p[2]; px[1] = p[1]; px[2] = p[0]; if (f->bpp == 4) px[3] = 0xFF; }
    else
    {
        u16 v;
        if (f->fmt == 3)      v = (u16)(((p[0] >> 3) << 11) | ((p[1] >> 3) << 6) | ((p[2] >> 3) << 1) | 1);
        else if (f->fmt == 4) v = (u16)(((p[0] >> 4) << 12) | ((p[1] >> 4) << 8) | ((p[2] >> 4) << 4) | 0xF);
        else                 v = (u16)(((p[0] >> 3) << 11) | ((p[1] >> 2) << 5) | (p[2] >> 3));
        *(volatile u16 *)px = v;
    }
}

static void BlitTopRect(const FbInfo *f, int x0, int y0, int w, int h)
{
    if (!f->fb) return;
    for (int x = x0; x < x0 + w; ++x)
        for (int y = y0; y < y0 + h; ++y)
            FbWritePx(f, x, y, CPix(x, y), TOP_H);
}

// Which buffer the top screen was showing when we took over. Present() flips this register to
// show OUR frame; nobody else ever flips it back, so the plugin has to hand it back itself.
static u32 g_lcdSelSaved = 0;
static int g_lcdSelValid = 0;

// Call when the overlay takes the top screen, BEFORE the first Present().
static void TopTakeOver(void)
{
    if (g_lcdSelValid) return;              // nested (quick menu -> menu): keep the outermost save
    g_lcdSelSaved = REG32(LCD_TOP + LCD_SELECT) & 1;
    g_lcdSelValid = 1;
}

// Call when handing the screen back to the game, right before ResumeGame().
//
// WHY THIS EXISTS: the bottom screen draws straight into the visible buffer, so restoring its
// pixels is enough. The top does NOT - Present() writes the hidden buffer and flips this
// register. Leave it flipped and the LCD keeps scanning our frame: the game runs (bottom screen
// returns, audio plays) while the top stays frozen on the menu.
static void TopRelease(void)
{
    if (!g_lcdSelValid) return;
    REG32(LCD_TOP + LCD_SELECT) = g_lcdSelSaved;
    g_lcdSelValid = 0;
}

static void Present(void)
{
    u32 sel = REG32(LCD_TOP + LCD_SELECT) & 1;
    FbInfo f = GetFb(1);
    if (!f.fb) return;
    BlitTopRect(&f, 0, 0, TOP_W, TOP_H);
    REG32(LCD_TOP + LCD_SELECT) = sel ^ 1;
}

static void GrabFb(void)
{
    FbInfo f = GetFb(0);
    if (!f.fb) { memset(gCompose, 0, TOP_W * TOP_H * 3); return; }
    for (int y = 0; y < TOP_H; ++y)
        for (int x = 0; x < TOP_W; ++x)
        {
            u32 off = (u32)x * f.stride + (u32)(TOP_H - 1 - y) * f.bpp;
            volatile u8 *px = (volatile u8 *)((f.fb + off) | (1u << 31));
            u8 r, g, b;
            if (f.bpp >= 3) { b = px[0]; g = px[1]; r = px[2]; }
            else
            {
                u16 v = *(volatile u16 *)px;
                if (f.fmt == 3)      { r = (u8)(((v >> 11) & 31) << 3); g = (u8)(((v >> 6) & 31) << 3); b = (u8)(((v >> 1) & 31) << 3); }
                else if (f.fmt == 4) { r = (u8)(((v >> 12) & 15) * 17); g = (u8)(((v >> 8) & 15) * 17); b = (u8)(((v >> 4) & 15) * 17); }
                else                 { r = (u8)(((v >> 11) & 31) << 3); g = (u8)(((v >> 5) & 63) << 2); b = (u8)((v & 31) << 3); }
            }
            u8 *p = CPix(x, y);
            p[0] = r; p[1] = g; p[2] = b;
        }
}

#include "plugin/cheat_ids.inc.c"

// ===================== Config persistence (SD, fs:USER) =====================
// fs:USER is in every game's ACL (unlike ir:rst), so a raw plugin can read/write the
// SD card. We persist favorites + toggles + hotkeys + theme next to the .3gx. Cheat
// state itself is NEVER persisted - never auto-enable a code patch on boot.
#define CFG_MAGIC 0x504D4343 // 'CCMP' - CTRComposer
#define CFG_VER   1

static FS_Archive cfgArchive;
static int fsReady;

// APPEND-ONLY: only add new fields at the END, never reorder or remove. ConfigLoad
// migrates older files instead of resetting them, so a new field must (a) bump
// CFG_VER and (b) get a default in ConfigLoad's `if (version < N)` ladder when its
// default is non-zero. Dropping the blob on a version bump resets every user.
typedef struct {
    u32 magic, version;
    u8  toast;
    u8  qmCombo;
    u8  theme;
    u8  lang;
    u8  hk1;      // example hotkey 1 (index into hotKeys[])
    u8  hk2;      // example hotkey 2
    u8  autoFill; // 1 = auto-fill the tracker from memory each time it opens
} ConfigBlob; // favorites live in Favorites.txt, so the cheat list can change without resetting them

static void FsBootInit(void)
{
    if (fsReady) return;
    if (R_FAILED(fsInit())) return;
    if (R_FAILED(FSUSER_OpenArchive(&cfgArchive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, "")))) return;
    fsReady = 1;
}

static void ConfigApply(const ConfigBlob *c); // fwd
static void ConfigFill(ConfigBlob *c);        // fwd
static void ApplyTheme(int idx);              // fwd (defined with the color state)
static void LangLoad(void);                   // fwd (defined below with localization)
static const char *HkExpand(const char *s, int cheat); // fwd
static int  MemReadable(u32 a);               // fwd (defined with the Hex Editor)
static int  MemWritable(u32 a);               // fwd

// ===================== Localization (gettext-style: English source = key) =====================
// UI strings are wrapped in T("English"). At runtime T() looks the English text up in
// a table loaded from <plugin dir>/lang/<Language>.txt and returns the translation, or
// the English string unchanged when there's no entry - so a partial translation just
// shows English for the missing lines instead of blanks.
//
// The template ships ENGLISH ONLY: no language files are included. The names below are
// simply the filenames the loader will look for, so a translator can drop in
// lang/Francais.txt and it works with no code change. Add or remove names freely.
static const char *kLangNames[] = {
    "English", "Francais", "Deutsch", "Italiano", "Espanol", "Portugues",
};
#define NUM_LANGS (int)(sizeof(kLangNames)/sizeof(kLangNames[0]))
// Menu label shown for each language (in its own language; ASCII-safe filename above).
static const char *kLangLabels[] = {
    "English", "Francais", "Deutsch", "Italiano", "Espanol", "Portugues (BR)",
};
static int g_langIdx = 0;
static int g_firstRun = 1;   // no Settings.cfg yet -> show the language chooser once

#define LANG_MAX 512
static char       *g_langBuf = NULL;         // whole file, parsed in place
static const char *g_enKey[LANG_MAX];
static const char *g_trVal[LANG_MAX];
static int         g_langCount = 0;

// Translate: return the localized form of an English UI string, or the input.
static const char *T(const char *en)
{
    if (!en || !g_langCount) return en;
    for (int i = 0; i < g_langCount; ++i)
        if (g_enKey[i] == en || strcmp(g_enKey[i], en) == 0) return g_trVal[i];
    return en;
}

// Load lang/<name>.txt into the table. Parses "English=Translation" lines,
// '#'/';'/blank lines ignored. English (idx 0) or any failure => identity table.
static void LangLoad(void)
{
    g_langCount = 0;
    if (g_langBuf) { free(g_langBuf); g_langBuf = NULL; }
    if (g_langIdx <= 0 || g_langIdx >= NUM_LANGS) return;

    FsBootInit();
    if (!fsReady) return;

    char leaf[64];
    int p = 0;
    for (const char *s = "lang/"; *s; ++s) leaf[p++] = *s;
    for (const char *s = kLangNames[g_langIdx]; *s; ++s) leaf[p++] = *s;
    for (const char *s = ".txt"; *s; ++s) leaf[p++] = *s;
    leaf[p] = 0;

    Handle f;
    if (R_FAILED(FSUSER_OpenFile(&f, cfgArchive, fsMakePath(PATH_ASCII, PlgPath(leaf)), FS_OPEN_READ, 0)))
        return;
    u64 sz64 = 0;
    FSFILE_GetSize(f, &sz64);
    u32 sz = (u32)sz64;
    if (sz == 0 || sz > 256 * 1024) { FSFILE_Close(f); return; }
    g_langBuf = (char *)malloc(sz + 1);
    if (!g_langBuf) { FSFILE_Close(f); return; }
    u32 got = 0;
    Result r = FSFILE_Read(f, &got, 0, g_langBuf, sz);
    FSFILE_Close(f);
    if (R_FAILED(r) || got == 0) { free(g_langBuf); g_langBuf = NULL; return; }
    g_langBuf[got] = 0;

    // Skip a UTF-8 BOM if present.
    char *q = g_langBuf;
    if ((u8)q[0] == 0xEF && (u8)q[1] == 0xBB && (u8)q[2] == 0xBF) q += 3;

    while (*q && g_langCount < LANG_MAX)
    {
        char *line = q;
        while (*q && *q != '\n') q++;
        char *eol = q;
        if (*q) q++;                       // step past '\n'
        if (eol > line && eol[-1] == '\r') eol[-1] = 0;
        *eol = 0;

        if (*line == '#' || *line == ';' || *line == 0) continue;
        char *eq = line;
        while (*eq && *eq != '=') eq++;
        if (*eq != '=') continue;          // no '=' -> not a mapping
        *eq = 0;
        char *val = eq + 1;
        if (*line == 0 || *val == 0) continue;  // empty key/value: keep English
        g_enKey[g_langCount] = line;
        g_trVal[g_langCount] = val;
        g_langCount++;
    }
}

// Which languages actually have a file on the SD card. English (idx 0) is embedded, so
// always available; the rest need lang/<Name>.txt to exist. The template ships none, so
// out of the box only English reads as available. Cheap enough to re-run on picker entry.
static u8 g_langAvail[NUM_LANGS];
static void LangProbeAvail(void)
{
    g_langAvail[0] = 1;
    FsBootInit();
    for (int i = 1; i < NUM_LANGS; ++i)
    {
        g_langAvail[i] = 0;
        if (!fsReady) continue;
        char leaf[64]; int p = 0;
        for (const char *s = "lang/"; *s; ++s) leaf[p++] = *s;
        for (const char *s = kLangNames[i]; *s; ++s) leaf[p++] = *s;
        for (const char *s = ".txt"; *s; ++s) leaf[p++] = *s;
        leaf[p] = 0;
        Handle f;
        if (R_SUCCEEDED(FSUSER_OpenFile(&f, cfgArchive, fsMakePath(PATH_ASCII, PlgPath(leaf)), FS_OPEN_READ, 0)))
        { g_langAvail[i] = 1; FSFILE_Close(f); }
    }
}

// ===================== SD-loaded guides (per language) =====================
// The English Game Guide + Plugin Guide stay embedded (guide.h / PLUGIN_PAGES) as the
// always-available fallback. For other languages we load translated copies from
// <plugin dir>/guide/<Name>/{game,plugin}.txt at runtime, so the binary stays small and
// guides are editable without recompiling.
//
// This is also how you ship a GAME guide without touching C: drop
// guide/English/game.txt on the SD card and it replaces the placeholder pages.
// File format: "%C Category" starts a category, "%P Page" starts a page; every other
// line is body text (kept verbatim, including its newlines).
#define SDG_MAXCATS  12
#define SDG_MAXPAGES 96

static char      *g_ggBuf;                    // game.txt (parsed in place)
static GuidePage  g_ggPagesBuf[SDG_MAXPAGES];
static GuideCat   g_ggCatsBuf[SDG_MAXCATS];
static int        g_ggNCats;                  // 0 -> use embedded GUIDE_CATS
static char      *g_pgBuf;                    // plugin.txt
static GuidePage  g_pgPagesBuf[32];
static int        g_pgNPages;                 // 0 -> use embedded PLUGIN_PAGES

// Read a whole SD text file into a fresh malloc'd, NUL-terminated buffer (or NULL).
static char *ReadSdText(const char *path, u32 maxsz)
{
    Handle f;
    if (R_FAILED(FSUSER_OpenFile(&f, cfgArchive, fsMakePath(PATH_ASCII, path), FS_OPEN_READ, 0)))
        return NULL;
    u64 sz64 = 0; FSFILE_GetSize(f, &sz64);
    u32 sz = (u32)sz64;
    if (sz == 0 || sz > maxsz) { FSFILE_Close(f); return NULL; }
    char *buf = (char *)malloc(sz + 1);
    if (!buf) { FSFILE_Close(f); return NULL; }
    u32 got = 0;
    Result r = FSFILE_Read(f, &got, 0, buf, sz);
    FSFILE_Close(f);
    if (R_FAILED(r) || got == 0) { free(buf); return NULL; }
    buf[got] = 0;
    return buf;
}

// Parse "%C/%P" markers in place into cats[]/pages[]. Bodies are terminated by
// NUL at the byte before the next marker. Returns category count.
static int ParseGuideCats(char *buf, GuideCat *cats, GuidePage *pages, int maxc, int maxp)
{
    int nc = 0, np = 0;
    GuideCat *cc = NULL; GuidePage *lp = NULL;
    char *p = buf;
    if ((u8)p[0] == 0xEF && (u8)p[1] == 0xBB && (u8)p[2] == 0xBF) p += 3; // skip BOM
    while (*p)
    {
        char *line = p;
        while (*p && *p != '\n') p++;
        char *eol = p; if (*p) p++;
        int isC = (line[0] == '%' && line[1] == 'C' && line[2] == ' ');
        int isP = (line[0] == '%' && line[1] == 'P' && line[2] == ' ');
        if (!(isC || isP)) continue;                 // body line: leave as-is
        if (lp && line > buf) line[-1] = 0;          // terminate previous page body
        char *t = eol; if (t > line && t[-1] == '\r') t--; *t = 0; // terminate title
        if (isC) { if (nc < maxc) { cc = &cats[nc++]; cc->title = line + 3; cc->pages = &pages[np]; cc->nPages = 0; } lp = NULL; }
        else if (cc && np < maxp) { lp = &pages[np++]; lp->title = line + 3; lp->body = p; cc->nPages++; }
    }
    return nc;
}

// Same, but flat pages only (Plugin Guide has no categories).
static int ParseGuidePages(char *buf, GuidePage *pages, int maxp)
{
    int np = 0; GuidePage *lp = NULL;
    char *p = buf;
    if ((u8)p[0] == 0xEF && (u8)p[1] == 0xBB && (u8)p[2] == 0xBF) p += 3;
    while (*p)
    {
        char *line = p;
        while (*p && *p != '\n') p++;
        char *eol = p; if (*p) p++;
        if (!(line[0] == '%' && line[1] == 'P' && line[2] == ' ')) continue;
        if (lp && line > buf) line[-1] = 0;
        char *t = eol; if (t > line && t[-1] == '\r') t--; *t = 0;
        if (np < maxp) { lp = &pages[np++]; lp->title = line + 3; lp->body = p; }
    }
    return np;
}

// Load SD guides for the current language. English also gets a look, so a dropped-in
// guide/English/game.txt overrides the embedded placeholder pages.
static void GuideLoad(void)
{
    g_ggNCats = 0; g_pgNPages = 0;
    if (g_ggBuf) { free(g_ggBuf); g_ggBuf = NULL; }
    if (g_pgBuf) { free(g_pgBuf); g_pgBuf = NULL; }
    if (g_langIdx < 0 || g_langIdx >= NUM_LANGS) return;
    FsBootInit();
    if (!fsReady) return;

    char leaf[128];
    for (int which = 0; which < 2; ++which)
    {
        int n = 0;
        for (const char *s = "guide/"; *s; ++s) leaf[n++] = *s;
        for (const char *s = kLangNames[g_langIdx]; *s; ++s) leaf[n++] = *s;
        leaf[n++] = '/';
        for (const char *s = (which == 0 ? "game.txt" : "plugin.txt"); *s; ++s) leaf[n++] = *s;
        leaf[n] = 0;

        char *buf = ReadSdText(PlgPath(leaf), 320 * 1024);
        if (!buf) continue;
        if (which == 0)
        {
            g_ggBuf = buf;
            g_ggNCats = ParseGuideCats(buf, g_ggCatsBuf, g_ggPagesBuf, SDG_MAXCATS, SDG_MAXPAGES);
            if (g_ggNCats <= 0) { free(buf); g_ggBuf = NULL; g_ggNCats = 0; }
        }
        else
        {
            g_pgBuf = buf;
            g_pgNPages = ParseGuidePages(buf, g_pgPagesBuf, 32);
            if (g_pgNPages <= 0) { free(buf); g_pgBuf = NULL; g_pgNPages = 0; }
        }
    }
}

#if !TOOLS_ONLY
static const GuideCat  *GG_Cats(int *n);   // fwd (defined with the Plugin Guide, after PLUGIN_PAGES)
#endif
static const GuidePage *PG_Pages(int *n);  // fwd

static void ConfigLoad(void)
{
    FsBootInit();
    if (!fsReady) return;
    Handle f;
    if (R_FAILED(FSUSER_OpenFile(&f, cfgArchive, fsMakePath(PATH_ASCII, PlgPath("Settings.cfg")), FS_OPEN_READ, 0)))
        return;
    // Migrate instead of reset: the blob is append-only, so an older/shorter file still has
    // valid theme/lang/hotkey fields. Read what's there (zero-filled) and accept any version
    // in [1, current]. When you add a field, give it its default in a `if (c.version < N)`
    // line here so existing users keep the rest of their settings.
    ConfigBlob c;
    memset(&c, 0, sizeof c);
    u32 got = 0;
    Result r = FSFILE_Read(f, &got, 0, &c, sizeof(c));
    FSFILE_Close(f);
    if (R_SUCCEEDED(r) && got >= 12 && c.magic == CFG_MAGIC && c.version >= 1 && c.version <= CFG_VER)
        ConfigApply(&c);
}

static void ConfigSave(void)
{
    FsBootInit();
    if (!fsReady) return;
    ConfigBlob c;
    ConfigFill(&c);
    Handle f;
    if (R_FAILED(FSUSER_OpenFile(&f, cfgArchive, fsMakePath(PATH_ASCII, PlgPath("Settings.cfg")),
                                 FS_OPEN_WRITE | FS_OPEN_CREATE, 0)))
        return;
    u32 wrote = 0;
    FSFILE_SetSize(f, sizeof(ConfigBlob));
    FSFILE_Write(f, &wrote, 0, &c, sizeof(c), FS_WRITE_FLUSH);
    FSFILE_Close(f);
}

// ===================== Quick menu hotkey (rebindable in Settings) =====================
typedef struct { const char *name; const char *plain; u32 pad; } QmCombo;
static const QmCombo qmCombos[] = {
    { "{L}+SELECT", "L+SELECT", BUTTON_L1 | BUTTON_SELECT }, // name: glyph-render; plain: toast/small font
    { "{R}+SELECT", "R+SELECT", BUTTON_R1 | BUTTON_SELECT },
};
#define NUM_QMCOMBOS (int)(sizeof(qmCombos) / sizeof(qmCombos[0]))
static int qmCombo = 0;

// Rebindable in-game hotkeys. A hold-to-act cheat reads its trigger from here instead
// of hard-coding a button, so the player can rebind it in Settings and the cheat's info
// card shows the LIVE binding as an inline glyph.
//
// B is deliberately absent from the default list in the reference plugin because it was
// the attack button in that game - pick the set that leaves your game playable.
typedef struct { const char *glyph; u32 mask; } HotKey;
static const HotKey hotKeys[] = {
    { "{A}", BUTTON_A }, { "{B}", BUTTON_B }, { "{X}", BUTTON_X }, { "{Y}", BUTTON_Y },
    { "{L}", BUTTON_L1 }, { "{R}", BUTTON_R1 },
    // ZL/ZR (New 3DS) are NOT here on purpose: they are not in the raw HID register we
    // poll (0x10146000), only in ir:rst / Circle Pad Pro shared memory, and most games
    // have no ir:rst in their exheader ACL. Assume they are unreachable. Use L/R combos.
};
#define NUM_HOTKEYS (int)(sizeof(hotKeys) / sizeof(hotKeys[0]))
#define HK1_DEFAULT 5 // {R} - Time Scrub, matching the AR code's own "hold R" convention
#define HK2_DEFAULT 4 // {L} - Day Scrub
static int hk1 = HK1_DEFAULT;
static int hk2 = HK2_DEFAULT;

// config <-> live state (defined here now that all the state exists)
static void ConfigApply(const ConfigBlob *c)
{
    cheatState[CH_CFG_TOAST] = c->toast ? 1 : 0;
    cheatState[CH_CFG_AUTOFILL] = c->autoFill ? 1 : 0;
    qmCombo = (c->qmCombo < NUM_QMCOMBOS) ? c->qmCombo : 0;
    hk1 = (c->hk1 < NUM_HOTKEYS) ? c->hk1 : HK1_DEFAULT;
    hk2 = (c->hk2 < NUM_HOTKEYS) ? c->hk2 : HK2_DEFAULT;
    ApplyTheme(c->theme);
    g_langIdx = (c->lang < NUM_LANGS) ? c->lang : 0;
    LangLoad();
    LangProbeAvail(); // know which languages have SD files (for the red "unavailable" marker)
    GuideLoad();
    g_firstRun = 0; // a valid config exists -> not the first launch
}
// Swap a {HK} token for the glyph token of whichever button the cheat's hotkey is bound to,
// so a label or info card always shows the LIVE binding - rebind it in Settings and the text
// follows. Strings with no {HK} are returned untouched, so this is safe to call on anything.
//
// Map your own hold-to-act cheats to their hotkey in the switch below.
static const char *HkExpand(const char *s, int cheat)
{
    static char buf[2][256];
    static int which = 0;
    int hk;
    switch (cheat)
    {
        case CH_EX_HOTKEY: hk = hk1; break;
        case CH_MM_TIME_SCRUB: hk = hk1; break;
        case CH_MM_DAY_SCRUB:  hk = hk2; break;
        default: return s;                  // not a hotkey cheat: nothing to substitute
    }
    if (!s) return s;
    const char *gl = hotKeys[hk].glyph;     // e.g. "{Y}" - the text routines draw it as an icon
    char *out = buf[which]; which ^= 1;
    int o = 0;
    for (int i = 0; s[i] && o < (int)sizeof(buf[0]) - 8; )
    {
        if (s[i] == '{' && s[i+1] == 'H' && s[i+2] == 'K' && s[i+3] == '}')
        { for (int k = 0; gl[k] && o < (int)sizeof(buf[0]) - 1; ++k) out[o++] = gl[k]; i += 4; }
        else out[o++] = s[i++];
    }
    out[o] = 0;
    return out;
}

static void ConfigFill(ConfigBlob *c)
{
    c->magic = CFG_MAGIC; c->version = CFG_VER;
    c->toast = cheatState[CH_CFG_TOAST];
    c->qmCombo = (u8)qmCombo;
    c->theme = (u8)g_themeIdx;
    c->lang = (u8)g_langIdx;
    c->hk1 = (u8)hk1;
    c->hk2 = (u8)hk2;
    c->autoFill = cheatState[CH_CFG_AUTOFILL];
}

#include "plugin/cheats.inc.c"

#include "plugin/pickers.inc.c"

#include "engine/menu_model.inc.c"

#include "plugin/menu_tables.inc.c"

#include "engine/menu_nav.inc.c"

#include "engine/favorites.inc.c"


// ===================== CTRPF-style rendering (themed window) =====================
#define WIN_X   40
#define WIN_Y   20
#define WIN_W   320
#define WIN_H   200
#define ROW_X   (WIN_X + 12)
#define ROW_W   (WIN_W - 24)
#define ROW_Y0  (WIN_Y + 32)
#define ROW_H   16
#define MAX_ROWS 9

// Live theme: the color macros expand to runtime arrays, so switching a theme
// just rewrites these and every CFill/CText call follows.
//
// These initializers mirror THEMES[0] only as a safety net. The AUTHORITY is the
// ApplyTheme(g_themeIdx) call in ThreadMain, which runs before anything is drawn - so a
// fresh install with no Settings.cfg still comes up on THEMES[0] instead of whatever
// happens to be hard-coded here. Do not rely on these values.
static u8 CINK[3]   = { 242, 242, 242 };
static u8 CDIM[3]   = { 150, 150, 150 };
static u8 CGOLD[3]  = { 230, 230, 230 };
static u8 CGREEN[3] = { 200, 200, 200 };
static u8 CBG[3]    = { 18, 18, 20 };
#define INK      CINK[0],   CINK[1],   CINK[2]
#define INK_DIM  CDIM[0],   CDIM[1],   CDIM[2]
#define GOLD     CGOLD[0],  CGOLD[1],  CGOLD[2]
#define GREEN_ON CGREEN[0], CGREEN[1], CGREEN[2]
#define BG       CBG[0],    CBG[1],    CBG[2]
// TRAP: each macro expands to THREE comma-separated arguments, so it only works in an
// argument position. NEVER put one in a ternary:
//     CText(x, y, s, on ? GREEN_ON : INK, 0);      // WRONG - silently miscolours
// `?:` binds tighter than `,`, so that parses as
//     CText(x, y, s, (on ? CGREEN[2] : CINK[0]), CINK[1], CINK[2], 0)
// which still compiles (the arg count happens to match) but takes green's BLUE channel and
// ink's green/blue. Pick the array instead, then index it:
//     const u8 *c = on ? CGREEN : CINK;
//     CText(x, y, s, c[0], c[1], c[2], 0);         // right

// Some themes have a light window bg; hardcoded light text then vanishes. Pick text
// colors from the bg luminance so overlays (e.g. the quick menu) stay readable everywhere.
static int ThemeBgLight(void) { return (CBG[0] * 30 + CBG[1] * 59 + CBG[2] * 11) / 100 > 140; }

static u8 ClampU8(int v) { return (u8)(v < 0 ? 0 : v > 255 ? 255 : v); }

// A recessed "inset" surface: form fields, value wells, progress troughs, status pills.
//
// Every one of these used to be a hardcoded olive literal (52,40,22 and friends) inherited from
// the original plugin's parchment palette. They ignored the theme completely, so switching to a
// dark theme left brown boxes scattered across the UI. Deriving them from CBG here means ONE
// place controls the look and every theme - including ones you add later - just works.
//
// dim != 0 flattens the surface back toward the background, for disabled/locked fields.
static void CFillInset(int x, int y, int w, int h, int dim)
{
    int lift = ThemeBgLight() ? -26 : 28;   // light theme: sink it. dark theme: raise it.
    if (dim) lift /= 3;
    CFill(x, y, w, h, ClampU8(CBG[0] + lift), ClampU8(CBG[1] + lift), ClampU8(CBG[2] + lift));
}

// A fixed dark inset field (status pills, dark tooltips) needs text that stays legible even when
// the active theme's accent is itself dark (a muted brown or navy accent, say).
// Lift a too-dark color toward white, preserving hue, so it reads on the ~{20,16,10} inset.
// Bright colors pass through unchanged, so it's safe to apply blindly.
#if !TOOLS_ONLY
static void LiftForDark(u8 r, u8 g, u8 b, u8 *o)
{
    int lum = (r * 30 + g * 59 + b * 11) / 100;
    if (lum >= 96) { o[0] = r; o[1] = g; o[2] = b; return; }
    int t = 96 - lum; if (t > 78) t = 78; // blend % toward white, capped so hue survives
    o[0] = (u8)(r + (255 - r) * t / 100);
    o[1] = (u8)(g + (255 - g) * t / 100);
    o[2] = (u8)(b + (255 - b) * t / 100);
}
#endif

static void ApplyTheme(int idx)
{
    if (idx < 0 || idx >= THEME_COUNT) idx = 0;
    const Theme *t = &THEMES[idx];
    for (int i = 0; i < 3; ++i)
    {
        CGOLD[i] = t->gold[i]; CINK[i] = t->ink[i]; CDIM[i] = t->dim[i];
        CGREEN[i] = t->green[i]; CBG[i] = t->bg[i];
    }
    g_themeIdx = idx; g_themeParchment = t->parchment;
}

static void DimOutsideWindow(void)
{
    for (int y = 0; y < TOP_H; ++y)
    {
        u8 *p = CPix(0, y);
        int inWinY = (y >= WIN_Y && y < WIN_Y + WIN_H);
        for (int x = 0; x < TOP_W; ++x, p += 3)
        {
            if (inWinY && x >= WIN_X && x < WIN_X + WIN_W) continue;
            p[0] = (u8)(p[0] * 130 / 255); p[1] = (u8)(p[1] * 130 / 255); p[2] = (u8)(p[2] * 130 / 255);
        }
    }
}

// Snapshot / restore the full top backdrop (dimmed game frame). Both screens
// share gCompose, so drawing the bottom can bleed into the top's out-of-window
// area; restoring the full backdrop before every top redraw keeps it clean.
static void CaptureTopBackdrop(void)
{
    if (!savedTop) return;
    for (int y = 0; y < TOP_H; ++y)
        for (int x = 0; x < TOP_W; ++x)
        {
            u8 *p = CPix(x, y);
            savedTop[y * TOP_W + x] = (u16)(((p[0] >> 3) << 11) | ((p[1] >> 2) << 5) | (p[2] >> 3));
        }
    savedTopValid = 1;
}
static void RestoreTopBackdrop(void)
{
    if (!savedTopValid) return;
    for (int y = 0; y < TOP_H; ++y)
        for (int x = 0; x < TOP_W; ++x)
        {
            u16 v = savedTop[y * TOP_W + x];
            u8 *p = CPix(x, y);
            p[0] = (u8)(((v >> 11) & 31) << 3); p[1] = (u8)(((v >> 5) & 63) << 2); p[2] = (u8)((v & 31) << 3);
        }
}

// The menu window: a flat themed fill with an accent border.
//
// A theme can instead ask for a background IMAGE by setting its `parchment` flag. The
// template ships no background art, so that path is stubbed out here. To use one: convert
// a BMP to an RGB565 C array (a small Python script - see Assets/), include it, and blit it
// where the marker below is. Embedding it in the .3gx avoids loose SD files.
static void ComposeBackdrop(void)
{
    RestoreTopBackdrop(); // full top from the saved backdrop (erases any bleed)

    if (g_themeParchment) // Termina: the Majora's Mask stained-glass art (topbg.h)
    {
        for (int y = 0; y < WIN_H; ++y)
        {
            const u16 *src = &topbg[y * TOPBG_W];
            u8 *p = CPix(WIN_X, WIN_Y + y);
            for (int x = 0; x < TOPBG_W; ++x, p += 3)
            {
                u16 v = src[x];
                p[0] = (u8)(((v >> 11) & 31) << 3);
                p[1] = (u8)(((v >> 5) & 63) << 2);
                p[2] = (u8)((v & 31) << 3);
            }
        }
        return;
    }

    CFill(WIN_X, WIN_Y, WIN_W, WIN_H, BG);
    CFill(WIN_X, WIN_Y, WIN_W, 2, GOLD); CFill(WIN_X, WIN_Y + WIN_H - 2, WIN_W, 2, GOLD);
    CFill(WIN_X, WIN_Y, 2, WIN_H, GOLD); CFill(WIN_X + WIN_W - 2, WIN_Y, 2, WIN_H, GOLD);
}

static void CheckBoxIcon(int x, int y, int on)
{
    CFillBlend(x, y, 12, 12, 0, 0, 0, 70);
    CFill(x, y, 12, 1, GOLD); CFill(x, y + 11, 12, 1, GOLD);
    CFill(x, y, 1, 12, GOLD); CFill(x + 11, y, 1, 12, GOLD);
    if (on)
    {
        for (int i = 0; i < 3; ++i) { CFill(x + 2 + i, y + 5 + i, 2, 2, GREEN_ON); }
        for (int i = 0; i < 5; ++i) { CFill(x + 4 + i, y + 8 - i, 2, 2, GREEN_ON); }
    }
}

// 12x12 placeholder for the main menu's checkbox column on rows that aren't on/off toggles
// (folders, pickers, tools, teleport rows, one-shot Settings actions...). Same fill as the
// checkbox's interior but with NO gold border, so it reads as "not a toggle" while still
// reserving the exact same width - keeps every row's icon/label lined up at the same x
// regardless of row type. Mirrors the quick menu's BrownBoxS at main-menu scale.
static void BrownBox(int x, int y)
{
    CFillBlend(x, y, 12, 12, 0, 0, 0, 70);
}

static void FolderIconSmall(int x, int y)
{
    CFill(x, y + 1, 6, 3, 172, 128, 34);
    CFill(x, y + 3, 13, 9, 219, 172, 66);
    CFill(x + 1, y + 4, 11, 2, 240, 205, 120);
    CFill(x, y + 3, 13, 1, 130, 92, 20);
}

// 9x9 checkbox for the compact quick menu
static void CheckBoxIconS(int x, int y, int on)
{
    CFillBlend(x, y, 9, 9, 0, 0, 0, 70);
    CFill(x, y, 9, 1, GOLD); CFill(x, y + 8, 9, 1, GOLD);
    CFill(x, y, 1, 9, GOLD); CFill(x + 8, y, 1, 9, GOLD);
    if (on)
    {
        CFill(x + 2, y + 4, 2, 2, GREEN_ON);
        CFill(x + 3, y + 5, 2, 2, GREEN_ON);
        for (int i = 0; i < 4; ++i) CFill(x + 4 + i, y + 5 - i, 2, 1, GREEN_ON);
    }
}
// 9x9 placeholder for the quick menu's left column on rows that are NOT on/off toggles
// (actions/shortcuts). Same fill as the checkbox's interior (black @ ~27% over the panel) but with
// NO gold border - so it reads as a solid tinted square, not a toggle. Theme-aware like the checkbox.
static void BrownBoxS(int x, int y)
{
    CFillBlend(x, y, 9, 9, 0, 0, 0, 70);
}
// True only for genuine on/off toggles - the cheats ApplyCheats() holds continuously.
// Everything else is a one-shot action or a shortcut, and gets a plain tinted box instead
// of a checkbox, so the menu never shows an "off" checkbox next to something that isn't
// stateful. KEEP THIS IN SYNC with the cheatState[] uses in ApplyCheats().
static int IsToggleCheat(int id)
{
    switch (id)
    {
        case CH_EX_DIRECT: case CH_EX_BYTE: case CH_EX_WORD:
        case CH_EX_BASEOFF: case CH_EX_HOTKEY:
        case CH_MM_RUPEES_MAX: case CH_MM_HEARTS_MAX: case CH_MM_DEFENSE:
        case CH_MM_AMMO_ARROWS: case CH_MM_AMMO_BOMBS: case CH_MM_AMMO_CHUS:
        case CH_MM_AMMO_STICKS: case CH_MM_AMMO_NUTS: case CH_MM_AMMO_BEANS:
        case CH_MM_AMMO_KEG: case CH_MM_TIME_SCRUB: case CH_MM_DAY_SCRUB:
        case CH_MM_MOONJUMP:
            return 1;
        default: return 0;
    }
}
static int C6Width(const char *s)
{
    int n = 0;
    while (*s) { if (SmallAscii(SysFontUtf8Next(&s))) n++; }
    return n * (FONT_WIDTH + 1);
}
// Generic RGBA4444 image blit with alpha, 1:1. Decodes each pixel (a = (v & 0xF) * 17)
// and blends it into the compose buffer. This is the base blit for all embedded art.
static void DrawImg(int x, int y, const unsigned short *px, int w, int h)
{
    for (int yy = 0; yy < h; ++yy)
        for (int xx = 0; xx < w; ++xx)
        {
            unsigned short v = px[yy * w + xx];
            u32 a = (u32)(v & 0xF) * 17;
            if (!a) continue;
            int X = x + xx, Y = y + yy;
            if ((unsigned)X >= TOP_W || (unsigned)Y >= TOP_H) continue;
            u8 r = (u8)(((v >> 12) & 0xF) * 17), g = (u8)(((v >> 8) & 0xF) * 17), b = (u8)(((v >> 4) & 0xF) * 17);
            u8 *p = CPix(X, Y);
            p[0] = (u8)((p[0] * (255 - a) + r * a) / 255);
            p[1] = (u8)((p[1] * (255 - a) + g * a) / 255);
            p[2] = (u8)((p[2] * (255 - a) + b * a) / 255);
        }
}
// Draw a 14px button glyph (A/B/X/Y/L/R/D-Pad) with alpha at (x,y) - just a fixed-size DrawImg.
static void DrawGlyph(int x, int y, int id)
{
    if (id < 0 || id >= NUM_GLYPHS) return;
    DrawImg(x, y, glyphs[id], GLY, GLY);
}

// Small-font text with inline button glyphs. Tokens: {A}{B}{X}{Y}{L}{R}{DP} become glyph icons.
// The glyph is drawn a touch above the text baseline so it centers on the ~10px line.
static int GlyphTok(const char *s) // returns glyph id if s points at a token, else -1
{
    if (s[0] != '{') return -1;
    if (s[1] == 'D' && s[2] == 'P' && s[3] == '}') return GL_DP;
    if (s[2] != '}') return -1;
    switch (s[1]) { case 'A': return GL_A; case 'B': return GL_B; case 'X': return GL_X;
                    case 'Y': return GL_Y; case 'L': return GL_L; case 'R': return GL_R; }
    return -1;
}
static void CText6Btn(int x, int y, const char *s, u8 r, u8 g, u8 b)
{
    while (*s)
    {
        int id = GlyphTok(s);
        if (id >= 0) { DrawGlyph(x, y - 3, id); x += GLY + 1; s += (id == GL_DP) ? 4 : 3; continue; }
        char one[2] = { *s, 0 };
        CText6(x, y, one, r, g, b);
        x += FONT_WIDTH + 1;
        s++;
    }
}
// Width in px of a CText6Btn string (glyph tokens count as GLY+1).
static int C6BtnWidth(const char *s)
{
    int w = 0;
    while (*s)
    {
        int id = GlyphTok(s);
        if (id >= 0) { w += GLY + 1; s += (id == GL_DP) ? 4 : 3; }
        else { w += FONT_WIDTH + 1; s++; }
    }
    return w;
}

// Large (system-font) text with inline 14px button glyphs. Same {A}{B}{X}{Y}{L}{R}{DP}
// tokens as CText6Btn; used on the pause help screen so controls show real 3DS buttons.
static void CTextBtn(int x, int y, const char *s, u8 r, u8 g, u8 b, int bold)
{
    char run[128];
    while (*s)
    {
        int n = 0;
        while (*s && GlyphTok(s) < 0 && n < 127) run[n++] = *s++;
        if (n) { run[n] = 0; CText(x, y, run, r, g, b, bold); x += CTextWidth(run); }
        int id = GlyphTok(s);
        if (id >= 0) { DrawGlyph(x, y + 1, id); x += GLY + 2; s += (id == GL_DP) ? 4 : 3; }
    }
}
// Pixel width of a CTextBtn string (glyph tokens count as GLY+2, text runs measured natively).
static int CTextBtnWidth(const char *s)
{
    int w = 0; char run[128];
    while (*s)
    {
        int n = 0;
        while (*s && GlyphTok(s) < 0 && n < 127) run[n++] = *s++;
        if (n) { run[n] = 0; w += CTextWidth(run); }
        int id = GlyphTok(s);
        if (id >= 0) { w += GLY + 2; s += (id == GL_DP) ? 4 : 3; }
    }
    return w;
}
// CTextBtn with truncation to maxw px (appends ".."). Tokens stay whole; only used for cheat labels.
static void CTextClipBtn(int x, int y, const char *s, int maxw, u8 r, u8 g, u8 b, int bold)
{
    if (CTextBtnWidth(s) <= maxw) { CTextBtn(x, y, s, r, g, b, bold); return; }
    int dots = CTextWidth("..");
    int cx = x;
    while (*s)
    {
        int id = GlyphTok(s);
        char one[2] = { *s, 0 };
        int uw = (id >= 0) ? (GLY + 2) : CTextWidth(one);
        if (cx + uw > x + maxw - dots) break;
        if (id >= 0) { DrawGlyph(cx, y + 1, id); s += (id == GL_DP) ? 4 : 3; }
        else         { CText(cx, y, one, r, g, b, bold); s++; }
        cx += uw;
    }
    CText(cx, y, "..", r, g, b, bold);
}

static void StarIcon(int x, int y)
{
    static const u8 rows[7] = { 0x08, 0x1C, 0x7F, 0x3E, 0x1C, 0x36, 0x63 };
    for (int r = 0; r < 7; ++r)
        for (int c = 0; c < 7; ++c)
            if (rows[r] & (0x40 >> c))
            {
                int X = x + c, Y = y + r;
                if ((unsigned)X < TOP_W && (unsigned)Y < TOP_H)
                { u8 *p = CPix(X, Y); p[0] = 255; p[1] = 214; p[2] = 90; }
            }
}

// ===================== Sprites (RGBA4444 art) =====================
// The template ships NO game art, so there is no sprite sheet here - but the machinery
// stays, because the machinery is the reusable half:
//
//   DrawImg()    - 1:1 alpha-blended blit of an RGBA4444 array (the button glyphs use it)
//   DrawScaled() - nearest-neighbour scaled blit: any source size onto any destination rect
//   the vector icons below - icons drawn from primitives: zero asset bytes, crisp at any
//                  size, and recolourable, so they never clash with a theme
//
// To add real art, convert a PNG to an RGBA4444 C array (v = R4<<12 | G4<<8 | B4<<4 | A4)
// and pack with ROUND-TO-NEAREST: clamp((c + 8) / 17, 0, 15). Truncating with c>>4 instead
// biases every channel upward by up to +15/255 and visibly brightens the art - very obvious
// on light tiles. Assets/gen_glyphs.py is a worked example of the correct packing.

// Nearest-neighbour scaled blit of an arbitrary RGBA4444 sprite (sw x sh) into a dst rect
// (dw x dh) on the top-screen compose buffer, alpha-blended. dim (0..255) darkens toward
// black - used to grey out the hex keys on the keypad while it is in DEC mode. One routine
// handles every art size, so a single small source tile can fill a large key.
static void DrawScaled(int dx, int dy, int dw, int dh, const u16 *px, int sw, int sh, int dim)
{
    for (int yy = 0; yy < dh; ++yy)
    {
        int sy = yy * sh / dh; int Y = dy + yy;
        if ((unsigned)Y >= TOP_H) continue;
        for (int xx = 0; xx < dw; ++xx)
        {
            int sx = xx * sw / dw; int X = dx + xx;
            if ((unsigned)X >= TOP_W) continue;
            u16 v = px[sy * sw + sx];
            u32 a = (u32)(v & 0xF) * 17;
            if (!a) continue;
            u8 r = (u8)(((v >> 12) & 0xF) * 17);
            u8 g = (u8)(((v >> 8) & 0xF) * 17);
            u8 b = (u8)(((v >> 4) & 0xF) * 17);
            if (dim) { r = (u8)(r * (255 - dim) / 255); g = (u8)(g * (255 - dim) / 255); b = (u8)(b * (255 - dim) / 255); }
            u8 *p = CPix(X, Y);
            p[0] = (u8)((p[0] * (255 - a) + r * a) / 255);
            p[1] = (u8)((p[1] * (255 - a) + g * a) / 255);
            p[2] = (u8)((p[2] * (255 - a) + b * a) / 255);
        }
    }
}

// Pseudo-keys for the code-drawn vector icons below. There is no sprite sheet in the
// template, so every key here maps to a function, not to art. Add your own the same way.
#define SPRK_SUNRISE  0x1F0
#define SPRK_DAY      0x1F1
#define SPRK_SUNSET   0x1F2
#define SPRK_NIGHT    0x1F3
#define SPRK_CLOCK    0x1F4
#define SPRK_RAIN     0x1F5
#define SPRK_PIN      0x1F7
#define SPRK_PORTAL   0x1F8
#define SPRK_MASK     0x1F9

// Which icon illustrates each cheat row (-1 = none, which is fine for most rows).
static int SpriteKeyForCheat(int ch)
{
    switch (ch)
    {
        case CH_EX_DIRECT:  return SPRK_PIN;
        case CH_EX_HOTKEY:  return SPRK_CLOCK;
        case CH_MM_DAY1: case CH_MM_DAY2: case CH_MM_DAY3: return SPRK_CLOCK;
        case CH_MM_TIME_6AM:  return SPRK_SUNRISE;
        case CH_MM_TIME_10AM: return SPRK_DAY;
        case CH_MM_TIME_6PM:  return SPRK_SUNSET;
        case CH_MM_TIME_SCRUB: return SPRK_CLOCK;
        case CH_MM_DAY_SCRUB:  return SPRK_DAY;

        case CH_MM_REFILL_HEARTS: case CH_MM_HEARTS_MAX: return MSPR_HEART;
        case CH_MM_REFILL_MAGIC:  return MSPR_MAGIC_FAIRY;
        case CH_MM_DEFENSE:       return MSPR_DEFENSE;

        case CH_MM_RUPEES_MAX:     return MSPR_RUPEE;
        case CH_MM_GILDED_MIRROR:  return MSPR_SWORD;
        case CH_MM_QUIVER_BOMBBAG: return MSPR_QUIVER;
        case CH_BBUTTON_GILDED:    return MSPR_SWORD;
        case CH_BBUTTON_GFSWORD:   return MSPR_GFSWORD;
        case CH_MM_BANK_FILL:      return MSPR_RUPEE;

        case CH_MM_AMMO_ARROWS: return MSPR_ARROWS;
        case CH_MM_AMMO_BOMBS:  return MSPR_BOMBS;
        case CH_MM_AMMO_CHUS:   return MSPR_BOMBCHUS;
        case CH_MM_AMMO_STICKS: return MSPR_STICKS;
        case CH_MM_AMMO_NUTS:   return MSPR_NUTS;
        case CH_MM_AMMO_BEANS:  return MSPR_BEANS;
        case CH_MM_AMMO_KEG:    return MSPR_KEG;

        case CH_MM_ALL_ITEMS:        return MSPR_ITEMS_DEED;
        case CH_MM_ALL_MASKS:        return MSPR_ALL_MASKS;
        case CH_MM_ALL_BOSSES_SONGS: return MSPR_BOSS_REMAINS;
        case CH_MM_ALL_FAIRIES:      return MSPR_FAIRY;
        case CH_TEST_FISHING:        return MSPR_FISHINGROD;

        case CH_MM_MOONJUMP: return MSPR_MOONJUMP;

        case CH_PLAY_NORMAL:      return SPRK_MASK; // no mask worn - generic hand-drawn mask icon
        case CH_PLAY_ZORA:        return MSPR_ZORA_MASK;
        case CH_PLAY_FIERCEDEITY: return MSPR_FD_MASK;

        // Settings rows: a mask changes your look, like a theme changes the menu's; a
        // written scroll for picking a language.
        case CH_CFG_THEME: return MSPR_GARO_MASK;
        case CH_CFG_LANG:  return MSPR_SCROLL;
    }
    return -1;
}

// ---- hand-drawn 16px icons (things the item sheet doesn't have) ----
static void CDisc(int cx, int cy, int r, u8 R, u8 G, u8 B)
{
    for (int dy = -r; dy <= r; ++dy)
        for (int dx = -r; dx <= r; ++dx)
            if (dx * dx + dy * dy <= r * r)
            {
                int X = cx + dx, Y = cy + dy;
                if ((unsigned)X < TOP_W && (unsigned)Y < TOP_H)
                { u8 *p = CPix(X, Y); p[0] = R; p[1] = G; p[2] = B; }
            }
}

static void DayIcon(int x, int y)
{
    CDisc(x + 8, y + 8, 4, 255, 214, 60);
    CFill(x + 8, y + 1, 1, 2, 255, 214, 60);  CFill(x + 8, y + 13, 1, 2, 255, 214, 60);
    CFill(x + 1, y + 8, 2, 1, 255, 214, 60);  CFill(x + 13, y + 8, 2, 1, 255, 214, 60);
    CFill(x + 3, y + 3, 2, 1, 255, 214, 60);  CFill(x + 11, y + 3, 2, 1, 255, 214, 60);
    CFill(x + 3, y + 12, 2, 1, 255, 214, 60); CFill(x + 11, y + 12, 2, 1, 255, 214, 60);
}

static void HalfSunIcon(int x, int y, u8 R, u8 G, u8 B) // sun on the horizon
{
    for (int dy = -4; dy <= 0; ++dy)
        for (int dx = -4; dx <= 4; ++dx)
            if (dx * dx + dy * dy <= 16)
            {
                int X = x + 8 + dx, Y = y + 11 + dy;
                if ((unsigned)X < TOP_W && (unsigned)Y < TOP_H)
                { u8 *p = CPix(X, Y); p[0] = R; p[1] = G; p[2] = B; }
            }
    CFill(x + 8, y + 3, 1, 2, R, G, B);       // ray up
    CFill(x + 3, y + 5, 2, 1, R, G, B);       // rays diagonal
    CFill(x + 11, y + 5, 2, 1, R, G, B);
    CFill(x + 1, y + 12, 14, 1, (u8)(R * 3 / 4), (u8)(G * 3 / 4), (u8)(B * 3 / 4)); // horizon
}

static void NightIcon(int x, int y)
{
    for (int dy = -5; dy <= 5; ++dy)
        for (int dx = -5; dx <= 5; ++dx)
        {
            if (dx * dx + dy * dy > 25) continue;
            int ox = dx - 3, oy = dy + 2;              // carve an offset disc -> crescent
            if (ox * ox + oy * oy <= 20) continue;
            int X = x + 8 + dx, Y = y + 8 + dy;
            if ((unsigned)X < TOP_W && (unsigned)Y < TOP_H)
            { u8 *p = CPix(X, Y); p[0] = 240; p[1] = 240; p[2] = 200; }
        }
    CFill(x + 3, y + 3, 1, 1, 255, 244, 180); // stars
    CFill(x + 5, y + 1, 1, 1, 255, 244, 180);
}

static void ClockIcon(int x, int y)
{
    for (int dy = -6; dy <= 6; ++dy)
        for (int dx = -6; dx <= 6; ++dx)
        {
            int rr = dx * dx + dy * dy;
            if (rr > 36 || rr < 25) continue;
            int X = x + 8 + dx, Y = y + 8 + dy;
            if ((unsigned)X < TOP_W && (unsigned)Y < TOP_H)
            { u8 *p = CPix(X, Y); p[0] = 236; p[1] = 200; p[2] = 120; }
        }
    CFill(x + 8, y + 4, 1, 4, 248, 240, 216); // hands
    CFill(x + 8, y + 8, 3, 1, 248, 240, 216);
}

static void RainIcon(int x, int y)
{
    CDisc(x + 5, y + 6, 3, 208, 212, 222);
    CDisc(x + 9, y + 5, 3, 208, 212, 222);
    CDisc(x + 11, y + 7, 2, 208, 212, 222);
    CFill(x + 3, y + 6, 11, 3, 208, 212, 222);
    CFill(x + 4, y + 11, 1, 3, 110, 170, 240);
    CFill(x + 8, y + 12, 1, 3, 110, 170, 240);
    CFill(x + 12, y + 11, 1, 3, 110, 170, 240);
}

static void PinIcon(int x, int y)
{
    CDisc(x + 8, y + 5, 4, 92, 202, 112);            // round head
    for (int i = 0; i < 7; ++i)                       // taper down to a point
    {
        int w = 7 - i; if (w < 1) w = 1;
        CFill(x + 8 - w / 2, y + 8 + i, w, 1, 92, 202, 112);
    }
    CDisc(x + 8, y + 5, 1, 26, 40, 30);               // hole in the head
}
// Warp: a teleport portal - two concentric rings + a bright core, cyan = "go there".
static void PortalIcon(int x, int y)
{
    for (int dy = -7; dy <= 7; ++dy)
        for (int dx = -7; dx <= 7; ++dx)
        {
            int rr = dx * dx + dy * dy;
            if (!((rr <= 49 && rr >= 32) || (rr <= 16 && rr >= 6))) continue; // outer + inner ring
            int X = x + 8 + dx, Y = y + 8 + dy;
            if ((unsigned)X < TOP_W && (unsigned)Y < TOP_H)
            { u8 *p = CPix(X, Y); p[0] = 96; p[1] = 196; p[2] = 236; }
        }
    CDisc(x + 8, y + 8, 1, 210, 244, 255);            // bright core
}
// Play as...: a stylized mask - round face, two eye holes, a mouth line. Hand-drawn rather than
// pulled from the sprite sheet, since none of the sheet's face cells could be confidently
// identified as Deku/Goron/Zora/Fierce Deity specifically (see gen_sprites_mm3d.py's notes).
static void MaskIcon(int x, int y)
{
    CDisc(x + 8, y + 8, 6, 214, 122, 40);             // mask body (Termina-orange)
    CDisc(x + 5, y + 6, 1, 24, 14, 8);                // left eye hole
    CDisc(x + 11, y + 6, 1, 24, 14, 8);               // right eye hole
    CFill(x + 6, y + 11, 5, 1, 24, 14, 8);             // mouth line
}

// ===================== Item sprites (RGBA4444, from sprites.h) =====================
static const SpriteRef *FindSprite(int key)
{
    if (key < 0) return NULL;
    for (int i = 0; i < NUM_SPRITES; ++i)
        if (sprites[i].key == key) return &sprites[i];
    return NULL;
}

static void DrawSprite(int x, int y, int key, int big)
{
    const SpriteRef *s = FindSprite(key);
    if (!s) return;
    const u16 *px = big ? s->px42 : s->px16;
    int n = big ? SPR42 : SPR16;
    for (int yy = 0; yy < n; ++yy)
        for (int xx = 0; xx < n; ++xx)
        {
            u16 v = px[yy * n + xx];
            u32 a = (u32)(v & 0xF) * 17;
            if (!a) continue;
            int X = x + xx, Y = y + yy;
            if ((unsigned)X >= TOP_W || (unsigned)Y >= TOP_H) continue;
            u8 r = (u8)(((v >> 12) & 0xF) * 17);
            u8 g = (u8)(((v >> 8) & 0xF) * 17);
            u8 b = (u8)(((v >> 4) & 0xF) * 17);
            u8 *p = CPix(X, Y);
            p[0] = (u8)((p[0] * (255 - a) + r * a) / 255);
            p[1] = (u8)((p[1] * (255 - a) + g * a) / 255);
            p[2] = (u8)((p[2] * (255 - a) + b * a) / 255);
        }
}

static void DrawCheatIcon(int x, int y, int ch)
{
    int sk = SpriteKeyForCheat(ch);
    switch (sk)
    {
        case SPRK_SUNRISE: HalfSunIcon(x, y, 255, 220, 90); return;
        case SPRK_DAY:     DayIcon(x, y);    return;
        case SPRK_SUNSET:  HalfSunIcon(x, y, 255, 140, 50); return;
        case SPRK_NIGHT:   NightIcon(x, y);  return;
        case SPRK_CLOCK:   ClockIcon(x, y);  return;
        case SPRK_RAIN:    RainIcon(x, y);   return;
        case SPRK_PIN:     PinIcon(x, y);    return;
        case SPRK_PORTAL:  PortalIcon(x, y); return;
        case SPRK_MASK:    MaskIcon(x, y);   return;
    }
    // >= 0x100: a real RGBA4444 sprite from sprites.h (see MM3D pseudo-ids below).
    if (sk >= 0) DrawSprite(x, y, sk, 0);
}

// ===================== Bottom screen =====================
// The bottom-screen window (control legend / tool forms). Anything outside it is the
// game's own frame, dimmed.
#define BWIN_X  20
#define BWIN_Y  20
#define BWIN_W  280
#define BWIN_H  200

static int savedBotValid;

static FbInfo GetBotFb(int which)
{
    u32 fmt = REG32(LCD_BOT + LCD_FORMAT) & 7;
    FbInfo f;
    f.fmt    = fmt;
    f.bpp    = (fmt == 0) ? 4u : (fmt == 1) ? 3u : 2u;
    f.stride = REG32(LCD_BOT + LCD_STRIDE);
    f.fb     = REG32(LCD_BOT + (which ? LCD_FBA2 : LCD_FBA1));
    return f;
}

static void BotGrab(void)
{
    u32 sel = REG32(LCD_BOT + LCD_SELECT) & 1;
    FbInfo f = GetBotFb(sel);
    savedBotValid = 0;
    if (!f.fb) return;
    for (int y = 0; y < BOT_H; ++y)
        for (int x = 0; x < BOT_W; ++x)
        {
            volatile u8 *px = (volatile u8 *)((f.fb + (u32)x * f.stride + (u32)(BOT_H - 1 - y) * f.bpp) | (1u << 31));
            u8 r, g, b;
            if (f.bpp >= 3) { b = px[0]; g = px[1]; r = px[2]; }
            else
            {
                u16 v = *(volatile u16 *)px;
                if (f.fmt == 3)      { r = (u8)(((v >> 11) & 31) << 3); g = (u8)(((v >> 6) & 31) << 3); b = (u8)(((v >> 1) & 31) << 3); }
                else if (f.fmt == 4) { r = (u8)(((v >> 12) & 15) * 17); g = (u8)(((v >> 8) & 15) * 17); b = (u8)(((v >> 4) & 15) * 17); }
                else                 { r = (u8)(((v >> 11) & 31) << 3); g = (u8)(((v >> 5) & 63) << 2); b = (u8)((v & 31) << 3); }
            }
            savedBot[y * BOT_W + x] = (u16)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
        }
    savedBotValid = 1;
}

static void BotRestoreBoth(void)
{
    if (!savedBotValid) return;
    for (int which = 0; which < 2; ++which)
    {
        FbInfo f = GetBotFb(which);
        if (!f.fb) continue;
        for (int y = 0; y < BOT_H; ++y)
            for (int x = 0; x < BOT_W; ++x)
            {
                u16 v = savedBot[y * BOT_W + x];
                u8 p[3] = { (u8)(((v >> 11) & 31) << 3), (u8)(((v >> 5) & 63) << 2), (u8)((v & 31) << 3) };
                FbWritePx(&f, x, y, p, BOT_H);
            }
    }
}

static void BotBlitComposeBoth(void)
{
    for (int which = 0; which < 2; ++which)
    {
        FbInfo f = GetBotFb(which);
        if (!f.fb) continue;
        for (int y = 0; y < BOT_H; ++y)
            for (int x = 0; x < BOT_W; ++x)
                FbWritePx(&f, x, y, CPix(x, y), BOT_H);
    }
}

static void ComposeBottom(void)
{
    for (int y = 0; y < BOT_H; ++y)
        for (int x = 0; x < BOT_W; ++x)
        {
            u8 *p = CPix(x, y);
            if (savedBotValid)
            {
                u16 v = savedBot[y * BOT_W + x];
                p[0] = (u8)(((v >> 11) & 31) << 3); p[1] = (u8)(((v >> 5) & 63) << 2); p[2] = (u8)((v & 31) << 3);
            }
            else { p[0] = p[1] = p[2] = 0; }
            if (x < BWIN_X || x >= BWIN_X + BWIN_W || y < BWIN_Y || y >= BWIN_Y + BWIN_H)
            { p[0] = (u8)(p[0] * 130 / 255); p[1] = (u8)(p[1] * 130 / 255); p[2] = (u8)(p[2] * 130 / 255); }
        }

    if (g_themeParchment) // Termina: the Majora's Mask stained-glass art (botbg.h)
    {
        for (int y = 0; y < BOTBG_H; ++y)
        {
            const u16 *src = &botbg[y * BOTBG_W];
            u8 *p = CPix(BWIN_X, BWIN_Y + y);
            for (int x = 0; x < BOTBG_W; ++x, p += 3)
            {
                u16 v = src[x];
                p[0] = (u8)(((v >> 11) & 31) << 3);
                p[1] = (u8)(((v >> 5) & 63) << 2);
                p[2] = (u8)((v & 31) << 3);
            }
        }
    }
    else // themed: solid background + accent border
    {
        CFill(BWIN_X, BWIN_Y, BWIN_W, BWIN_H, BG);
        CFill(BWIN_X, BWIN_Y, BWIN_W, 2, GOLD); CFill(BWIN_X, BWIN_Y + BWIN_H - 2, BWIN_W, 2, GOLD);
        CFill(BWIN_X, BWIN_Y, 2, BWIN_H, GOLD); CFill(BWIN_X + BWIN_W - 2, BWIN_Y, 2, BWIN_H, GOLD);
    }

    int tx = BWIN_X + 18, ty = BWIN_Y + 14;
    CText(tx, ty, PLUGIN_NAME, GOLD, 1);
    CFill(tx, ty + 17, CTextWidth(PLUGIN_NAME) + 8, 1, GOLD);
    CTextBtn(tx, ty + 30,  T("{DP} navigate / page"), INK, 0);
    CTextBtn(tx, ty + 48,  T("{A} open / toggle    {B} back"), INK, 0);
#if TOOLS_ONLY
    CTextBtn(tx, ty + 66,  T("{X} info    {Y} favorite"), INK, 0);
#else
    CTextBtn(tx, ty + 66,  T("{X} cheat info    {Y} favorite"), INK, 0);
#endif
    CText(tx, ty + 84,  T("SELECT: close menu"), INK, 0);
    // quick-menu hint on two lines, dropped a touch below SELECT so it isn't crowded, and short enough
    // that the combo line stays left and never runs off the parchment.
    CText(tx, ty + 110, T("in-game:"), INK_DIM, 0);
    {
        char qm[64]; int i = 0;
        for (const char *s = qmCombos[qmCombo].name; *s && i < 12; ++s) qm[i++] = *s;
        qm[i++] = ' ';
        for (const char *s = T("quick menu"); *s && i < 62; ++s) qm[i++] = *s;
        qm[i] = 0;
        CTextBtn(tx, ty + 126, qm, GOLD, 0);
    }
    CText6(tx, BWIN_Y + BWIN_H - 16, PLUGIN_NAME " " PLUGIN_VER, INK_DIM);
}

// ===================== Toast =====================
static char toastMsg[48];
static volatile int toastTicks;

static void QueueToastRaw(const char *label, const char *suffix)
{
    if (!cheatState[CH_CFG_TOAST]) return;
    int i = 0;
    while (label[i] && i < 38) { toastMsg[i] = label[i]; i++; }
    for (int j = 0; suffix[j] && i < 46; ++j) toastMsg[i++] = suffix[j];
    toastMsg[i] = 0;
    toastTicks = 625; // ~2.5s at the 4ms toast tick
}
static void QueueToast(const char *label, int on) { QueueToastRaw(label, on ? ": ON" : ": OFF"); }

static void ToastTick(void)
{
    if (toastTicks <= 0) return;
    toastTicks--;

    int w = C6BtnWidth(toastMsg) + 10, h = 14; // glyph-aware: cheat labels may carry {A}/{L} tokens
    int x0 = TOP_W - 6 - w, y0 = TOP_H - 6 - h;

    CFill(x0, y0, w, h, BG); // theme background: keeps text readable on light & dark themes
    CFill(x0, y0, w, 1, GOLD); CFill(x0, y0 + h - 1, w, 1, GOLD);
    CFill(x0, y0, 1, h, GOLD); CFill(x0 + w - 1, y0, 1, h, GOLD);
    CText6Btn(x0 + 5, y0 + 2, toastMsg, INK);

    FbInfo f = GetFb(0);
    BlitTopRect(&f, x0, y0, w, h);
}

// ===================== Menu rendering =====================
// ---- 13px themed category icons (gold) ----
#define GLD2 224, 186, 96
#define GLDK 130, 92, 20
// ---- generic vector tool icons -------------------------------------------------------
// Drawn from primitives: no asset bytes, crisp at any size, and they never clash with a
// theme. This is the art-free alternative to a sprite sheet - swap in DrawScaled() calls
// here if you would rather ship real art.
//
// The four tool icons below are currently unreferenced: this plugin ships real MM3D sprites for
// Cheat Search / RAM Dumper / Hex Editor / About instead. Kept as the art-free fallback (and as
// the reference for anyone forking the engine), marked unused so -Wall stays quiet.
// --gc-sections drops them from the binary either way.
__attribute__((unused)) static void MagnifierIcon(int x, int y)   // Cheat Search: lens + handle
{
    CDisc(x + 6, y + 6, 4, GOLD);
    CDisc(x + 6, y + 6, 2, BG);
    CFill(x + 9, y + 9, 2, 2, GOLD);
    CFill(x + 10, y + 10, 3, 3, GOLD);
}
__attribute__((unused)) static void DiskIcon(int x, int y)        // RAM Dumper: a save/disk block
{
    CFill(x + 2, y + 2, 12, 12, GOLD);
    CFill(x + 4, y + 3, 8, 4, BG);       // shutter
    CFill(x + 4, y + 9, 8, 4, BG);       // label
}
__attribute__((unused)) static void GridIcon(int x, int y)        // Hex Editor: a byte grid
{
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            CFill(x + 2 + c * 4, y + 3 + r * 4, 3, 3, GOLD);
}
__attribute__((unused)) static void InfoIcon(int x, int y)        // About: an "i" in a disc
{
    CDisc(x + 7, y + 7, 6, GOLD);
    CFill(x + 6, y + 3, 2, 2, BG);
    CFill(x + 6, y + 6, 2, 6, BG);
}
static void BookIcon(int x, int y)        // Guides: an open book
{
    CFill(x + 1, y + 3, 6, 10, GOLD);
    CFill(x + 8, y + 3, 6, 10, GOLD);
    CFill(x + 7, y + 2, 1, 12, INK_DIM);  // spine
    for (int r = 0; r < 3; ++r)
    {
        CFill(x + 2, y + 5 + r * 3, 4, 1, BG);
        CFill(x + 9, y + 5 + r * 3, 4, 1, BG);
    }
}
static void GearIcon(int x, int y)        // Settings rows
{
    CDisc(x + 7, y + 7, 5, GOLD);
    CDisc(x + 7, y + 7, 2, BG);
    CFill(x + 6, y + 1, 2, 2, GOLD); CFill(x + 6, y + 12, 2, 2, GOLD);
    CFill(x + 1, y + 6, 2, 2, GOLD); CFill(x + 12, y + 6, 2, 2, GOLD);
}

// Icon for a folder row. HOME-level MM3D folders use real sprites (same "obvious where
// possible, playful where not" logic as the cheat icons): Time -> Ocarina, Battle -> the
// Gilded Sword, Inventory -> a Rupee Wallet, Quest -> the Bombers' Notebook (literally MM3D's
// own quest-tracking item), Misc -> the Pictograph Box (a fun, novelty-flavored camera, for
// the novelty-cheats folder), Items (ammo) -> the crossed Arrows. Tools/Settings keep their
// generic engine vector icons.
static void CategoryIcon(int folderId, int x, int y)
{
    switch (folderId)
    {
#if !TOOLS_ONLY
        case F_TOOLS:      DrawSprite(x, y, MSPR_LENS, 0); break;
        case F_TIME:       DrawSprite(x, y, MSPR_OCARINA, 0); break;
        case F_BATTLE:     DrawSprite(x, y, MSPR_SWORD, 0); break;
        case F_INVENTORY:  DrawSprite(x, y, MSPR_WALLET, 0); break;
        case F_QUEST:      DrawSprite(x, y, MSPR_ALL_ITEMS, 0); break;
        case F_MISC:       DrawSprite(x, y, MSPR_CAMERA, 0); break;
        case F_TELEPORT:   PortalIcon(x, y - 1); break;
        case F_AMMO:       DrawSprite(x, y, MSPR_ARROWS, 0); break;
        case F_BOTTLES:    DrawSprite(x, y, MSPR_BOTTLE, 0); break;
#endif
        case F_SETTINGS: GearIcon(x, y - 1); break;
        default:         FolderIconSmall(x, y + 1); break;
    }
}

// Row icon for a tool (shared by the main menu and the quick menu).
static void ToolIcon(int tool, int x, int y)
{
    switch (tool)
    {
        case T_SEARCH:      DrawSprite(x, y, MSPR_LENS, 0);    return;
        case T_RAMDUMP:     DrawSprite(x, y, MSPR_MAP, 0);     return;
        case T_HEXEDIT:     DrawSprite(x, y, MSPR_COMPASS, 0); return;
        case T_ABOUT:       DrawSprite(x, y, MSPR_ABOUT_ICON, 0); return;
#if !TOOLS_ONLY
        case T_GAMEGUIDE:   BookIcon(x, y);      return;
        case T_TRACKER:     DrawSprite(x, y, MSPR_ALL_ITEMS, 0); return; // Bombers' Notebook - MM3D's own in-game checklist item
#endif
        case T_PLUGINGUIDE: BookIcon(x, y);      return;
        default:            FolderIconSmall(x, y + 2); return;
    }
}

// Draw one menu row/cell at (x,y) spanning cellW. Handles every item type.
//
// This is the layout-agnostic primitive: it renders ONE row wherever you place it, so the
// caller decides whether a folder is a list, a 2-column grid, or something else entirely.
//
// `checkboxAlign`: reserve a checkbox-width slot before the icon, like toggle/one-shot cheat
// rows do, so non-toggle rows (folders, pickers, tools...) line up with them. Only matters when
// a list MIXES row types (e.g. Inventory: cheat rows next to a "Bottles" folder row) - the
// 2-column grids (HOME, Bottles, unfiltered Teleport) are always one uniform row type, so they
// pass 0 here and keep their original tight icon-hugs-the-selector layout.
static void DrawMenuItem(const Item *it, int x, int y, int cellW, int selected, int checkboxAlign)
{
    int iconX = checkboxAlign ? x + 17 : x;           // icon x
    int lblX  = checkboxAlign ? x + 37 : x + 20;      // label x
    int padN  = checkboxAlign ? 43 : 26;              // label width padding (no value on the right)
    int padV  = checkboxAlign ? 49 : 32;              // label width padding (value shown on the right)
    if (IS_SEP(it)) // non-selectable section header (dim label + hairline rule)
    {
        const char *sec = T(it->label);
        CText6(x, y + 3, sec, INK_DIM);
        int lx = x + C6Width(sec) + 6;
        CFill(lx, y + 6, (x + cellW) - lx, 1, GOLD);
        return;
    }
    if (selected)
    {
        CFillBlend(x - 4, y - 1, cellW + 8, ROW_H, 0, 0, 0, 110);
        CFill(x - 4, y - 1, 2, ROW_H, GOLD);
    }
    if (it->folder >= 0)
    {
        if (checkboxAlign) BrownBox(x, y + 1);
        CategoryIcon(it->folder, iconX, y);
        CTextClip(lblX, y - 1, T(it->label), cellW - padN - (folderFav[it->folder] ? 12 : 0), INK, 0);
        if (folderFav[it->folder]) StarIcon(x + cellW - 12, y + 3);
    }
    else if (it->warp == -2) // Teleport category filter row (cycles with A)
    {
        if (checkboxAlign) BrownBox(x, y + 1);
        DrawSprite(iconX, y - 1, MSPR_LENS, 0); // Lens of Truth = filter / inspect
        const char *m = g_tpFilter == 1 ? T("Overworld") : g_tpFilter == 2 ? T("Dungeons") : T("All");
        int cw = CTextWidth(m);
        CText(x + cellW - 6 - cw, y - 1, m, GREEN_ON, 0);
        CTextClip(lblX, y - 1, T("Filter"), cellW - padV - cw, INK, 0);
    }
    else if (it->warp >= 0) // teleport destination: map-pin icon + name (from warps[])
    {
        if (checkboxAlign) BrownBox(x, y + 1);
        PinIcon(iconX, y - 1);
        CTextClip(lblX, y - 1, T(warps[it->warp].name), cellW - padN, INK, 0);
    }
    else if (it->picker >= 0)
    {
        const Picker *pk = &pickers[it->picker];
        u8 cur;
        int haveCur = PickerRead(pk, &cur), curOpt = -1;
        if (haveCur)
            for (int k = 0; k < pk->count; ++k)
                if (pk->opts[k].val == cur) { curOpt = k; break; }

        if (checkboxAlign) BrownBox(x, y + 1);
        // Row icon: the CURRENT value's own sprite when we have one (so the row itself shows
        // what's actually equipped/filled), else the picker's generic fallback.
        if (curOpt >= 0 && pk->opts[curOpt].icon >= 0) DrawSprite(iconX, y - 1, pk->opts[curOpt].icon, 0);
        else DrawSprite(iconX, y - 1, MSPR_BOTTLE, 0); // every remaining picker is a Bottle slot

        if (curOpt >= 0)
        {
            int cw = CTextWidth(T(pk->opts[curOpt].name));
            CText(x + cellW - 6 - cw, y - 1, T(pk->opts[curOpt].name), GREEN_ON, 0);
            CTextClip(lblX, y - 1, T(it->label), cellW - padV - cw, INK, 0);
        }
        else
            CTextClip(lblX, y - 1, T(it->label), cellW - padN, INK, 0);
    }
    else if (it->tool >= 0)
    {
        int fav = toolFav[it->tool], rpad = fav ? 14 : 0;
        if (checkboxAlign) BrownBox(x, y + 1);
        ToolIcon(it->tool, iconX, y - 1);
        CTextClip(lblX, y - 1, T(it->label), cellW - padN - rpad, GOLD, 0);
        if (fav) StarIcon(x + cellW - 12, y + 3);
    }
    else if (it->cheat == CH_CFG_QMKEY)
    {
        if (checkboxAlign) BrownBox(x, y + 1);
        GearIcon(iconX, y - 1);
        int cw = CTextBtnWidth(qmCombos[qmCombo].name); // button combo: L/R shown as glyphs
        CTextBtn(x + cellW - 6 - cw, y - 1, qmCombos[qmCombo].name, GREEN_ON, 0);
        CTextClip(lblX, y - 1, T(it->label), cellW - padV - cw, INK, 0);
    }
    else if (it->cheat == CH_CFG_HK1 || it->cheat == CH_CFG_HK2)
    {
        if (checkboxAlign) BrownBox(x, y + 1);
        GearIcon(iconX, y - 1);
        // Show the LIVE binding as a button glyph, so rebinding updates the row instantly.
        const char *g = hotKeys[it->cheat == CH_CFG_HK1 ? hk1 : hk2].glyph;
        int cw = CTextBtnWidth(g);
        CTextBtn(x + cellW - 6 - cw, y - 1, g, GREEN_ON, 0);
        CTextClip(lblX, y - 1, T(it->label), cellW - padV - cw, INK, 0);
    }
    else if (it->cheat == CH_CFG_HKRESET) // an action row: icon + label, no value/checkbox
    {
        if (checkboxAlign) BrownBox(x, y + 1);
        GearIcon(iconX, y - 1);
        CTextClip(lblX, y - 1, T(it->label), cellW - padN, GOLD, 0);
    }
    else if (it->cheat == CH_CFG_THEME)
    {
        if (checkboxAlign) BrownBox(x, y + 1);
        GearIcon(iconX, y - 1);
        int cw = CTextWidth(THEMES[g_themeIdx].name); // theme name: proper noun, not translated
        CText(x + cellW - 6 - cw, y - 1, THEMES[g_themeIdx].name, GREEN_ON, 0);
        CTextClip(lblX, y - 1, T(it->label), cellW - padV - cw, INK, 0);
    }
    else if (it->cheat == CH_CFG_LANG)
    {
        if (checkboxAlign) BrownBox(x, y + 1);
        GearIcon(iconX, y - 1);
        int cw = CTextWidth(kLangLabels[g_langIdx]); // language name: shown natively, not translated
        // red when this language has no SD file (falls back to English) - an availability cue
        if (g_langAvail[g_langIdx]) CText(x + cellW - 6 - cw, y - 1, kLangLabels[g_langIdx], GREEN_ON, 0);
        else                        CText(x + cellW - 6 - cw, y - 1, kLangLabels[g_langIdx], 225, 60, 45, 0);
        CTextClip(lblX, y - 1, T(it->label), cellW - padV - cw, INK, 0);
    }
    else if (it->cheat == CH_CFG_TOAST || it->cheat == CH_CFG_AUTOFILL) // settings toggles: no cheat icon
    {
        int on = cheatState[it->cheat] || flashCheat == it->cheat;
        CheckBoxIcon(x, y + 1, on);
        const u8 *c = on ? CGREEN : CINK;
        CTextClipBtn(x + 20, y - 1, T(it->label), cellW - 26, c[0], c[1], c[2], 0);
    }
    else
    {
        int on = cheatState[it->cheat] || flashCheat == it->cheat;
        CheckBoxIcon(x, y + 1, on);
        DrawCheatIcon(x + 17, y - 1, it->cheat);
        const u8 *c = on ? CGREEN : CINK;
        // HkExpand so a {HK} in the label renders as the live button glyph instead of the
        // literal token - CTextClipBtn then draws it as an icon like any other {A}/{B}.
        CTextClipBtn(x + 37, y - 1, HkExpand(T(it->label), it->cheat), cellW - 53, c[0], c[1], c[2], 0);
        if (flashCheat == it->cheat)
        {
            int fx = x + cellW - 6 - C6Width(flashMsg);
            if (flashMsg[0] == 'R') CText6(fx, y + 2, flashMsg, GOLD);       // REMOVED -> gold
            else                    CText6(fx, y + 2, flashMsg, GREEN_ON);   // ADDED / OK -> green
        }
        else if (favorite[it->cheat]) StarIcon(x + cellW - 12, y + 3);
    }
}

// HOME two-column grid layout: interleaves non-selectable section headers with
// pairs of items. Each item gets a (y, col); separators get a header row.
#define RL_MAX  40
#define HDR_H   14
static int g_rlY[RL_MAX], g_rlCol[RL_MAX], g_rlSep[RL_MAX], g_rlHid[RL_MAX];
static void BuildRootLayout(const Folder *fld)
{
    int folderIdx = (int)(fld - folders);
    int y = ROW_Y0, col = 0;
    for (int i = 0; i < fld->count && i < RL_MAX; ++i)
    {
        if (ItemHidden(folderIdx, &fld->items[i])) // filtered out: takes no grid slot
        { g_rlHid[i] = 1; g_rlSep[i] = 0; g_rlCol[i] = -1; g_rlY[i] = y; continue; }
        g_rlHid[i] = 0;
        if (IS_SEP(&fld->items[i]))
        {
            if (col != 0) { y += ROW_H; col = 0; } // finish an open pair row
            g_rlSep[i] = 1; g_rlCol[i] = -1; g_rlY[i] = y;
            y += HDR_H;
        }
        else if (fld->items[i].wide)
        {
            if (col != 0) { y += ROW_H; col = 0; } // finish an open pair row
            g_rlSep[i] = 0; g_rlCol[i] = -2; g_rlY[i] = y; // -2 = spans both columns, still selectable
            y += ROW_H;
        }
        else
        {
            g_rlSep[i] = 0; g_rlCol[i] = col; g_rlY[i] = y;
            if (++col == 2) { col = 0; y += ROW_H; }
        }
    }
}
// first / next selectable item index (skip separators and filtered-out rows)
static int RootFirstSel(const Folder *fld)
{
    int folderIdx = (int)(fld - folders);
    for (int i = 0; i < fld->count; ++i) if (!IS_SEP(&fld->items[i]) && !ItemHidden(folderIdx, &fld->items[i])) return i;
    return 0;
}
// grid neighbor of `cur` in a direction (0 up,1 down,2 left,3 right), skipping
// separators; up/down wrap within the same column. Needs BuildRootLayout first.
static int RootNeighbor(const Folder *fld, int cur, int dir)
{
    int cy = g_rlY[cur], cc = g_rlCol[cur]; // cc: 0/1 normal column, -2 = wide (spans both)
    if (dir == 2 || dir == 3)
    {
        if (cc < 0) return cur; // wide row: no left/right target within the same row
        int want = (dir == 3) ? 1 : 0;
        if (cc == want) return cur;
        for (int i = 0; i < fld->count; ++i)
            if (!g_rlSep[i] && !g_rlHid[i] && g_rlY[i] == cy && g_rlCol[i] == want) return i;
        return cur;
    }
    // Column filter: a normal column only matches its own column or a wide row (which spans
    // both); a wide row (cc<0) matches everything, so up/down passes through it either way.
    int best = -1, bestY = (dir == 1) ? 0x7fffffff : -1;
    for (int i = 0; i < fld->count; ++i)
    {
        if (g_rlSep[i] || g_rlHid[i]) continue;
        if (cc >= 0 && g_rlCol[i] >= 0 && g_rlCol[i] != cc) continue;
        int yy = g_rlY[i];
        if (dir == 1 && yy > cy && yy < bestY) { bestY = yy; best = i; }
        if (dir == 0 && yy < cy && yy > bestY) { bestY = yy; best = i; }
    }
    if (best >= 0) return best;
    int ext = (dir == 1) ? 0x7fffffff : -1, extI = cur;
    for (int i = 0; i < fld->count; ++i) // wrap to the far end of this column
    {
        if (g_rlSep[i] || g_rlHid[i]) continue;
        if (cc >= 0 && g_rlCol[i] >= 0 && g_rlCol[i] != cc) continue;
        int yy = g_rlY[i];
        if (dir == 1 && yy < ext) { ext = yy; extI = i; }
        if (dir == 0 && yy > ext) { ext = yy; extI = i; }
    }
    return extI;
}

static void ComposeMenu(const Folder *fld, int depth, int cursor, int scroll)
{
    ComposeBackdrop();

    int tw = CTextWidth(T(fld->title));
    CText(WIN_X + 12, WIN_Y + 7, T(fld->title), INK, 1);
    CFill(WIN_X + 12, WIN_Y + 24, tw + 6, 1, GOLD);

    CText6(WIN_X + WIN_W - 12 - C6Width(PLUGIN_TAG), WIN_Y + 8, PLUGIN_TAG, INK_DIM);

    // HOME uses a grouped two-column grid; every other folder is a plain scrolling list. This is
    // a LAYOUT CHOICE, not an engine rule - add folders to this test, or drop the grid entirely
    // and let everything be a list. When a grid folder overflows, `scroll` is a pixel offset and
    // rows are clipped and arrowed.
    // Teleport only grids when unfiltered: filtering to Overworld/Dungeons leaves most of its
    // category headers with just one surviving destination each, and a forced 2-col break per
    // header wasted the other column and forced a lot of scrolling for very little content -
    // a single full-width column both fixes that and stops long dungeon names from clipping.
    int twoCol = (fld == &folders[F_ROOT] || (fld == &folders[F_TELEPORT] && g_tpFilter == 0));
    if (twoCol)
    {
        BuildRootLayout(fld);
        int colW = ROW_W / 2;
        int visBot = WIN_Y + WIN_H - 22; // bottom of the content band (above the footer)
        int maxY = ROW_Y0;
        for (int i = 0; i < fld->count; ++i)
        {
            if (g_rlHid[i]) continue;
            int rowH = g_rlSep[i] ? HDR_H : ROW_H;
            if (g_rlY[i] + rowH > maxY) maxY = g_rlY[i] + rowH;
            int dy = g_rlY[i] - scroll;
            if (dy < ROW_Y0 - 2 || dy + rowH > visBot + 2) continue; // clip to the visible band
            if (g_rlSep[i])
            {
                const char *sec = T(fld->items[i].label);
                CText6(ROW_X, dy + 3, sec, 150, 140, 112); // small dim label
                int lx = ROW_X + C6Width(sec) + 6;
                CFill(lx, dy + 6, (WIN_X + WIN_W - 14) - lx, 1, 120, 98, 50); // hairline rule
            }
            else if (g_rlCol[i] == -2) // wide row: full row width, no column offset
                DrawMenuItem(&fld->items[i], ROW_X, dy, ROW_W - 8, i == cursor, 0);
            else
                DrawMenuItem(&fld->items[i], ROW_X + g_rlCol[i] * colW, dy, colW - 8, i == cursor, 0);
        }
        if (scroll > 0) // up arrow
            for (int a = 0; a < 4; ++a) CFill(WIN_X + WIN_W - 14 - a, ROW_Y0 + 3 + a, 1 + 2 * a, 1, GOLD);
        if (maxY - scroll > visBot) // down arrow
            for (int a = 0; a < 4; ++a) CFill(WIN_X + WIN_W - 14 - a, visBot - 4 - a, 1 + 2 * a, 1, GOLD);
    }
    else
    {
        // scroll is in VISIBLE rows (hidden items compacted away). Identical to raw indexing for
        // folders with nothing hidden, so only the Teleport filter changes behaviour here.
        int folderIdx = (int)(fld - folders);
        // Only Inventory mixes generic checkbox+icon cheat rows with folder/picker rows - every
        // other list is one uniform row type (Settings: gear icon or its own checkbox, always at
        // the same x either way; Quest/Ammo/Misc: cheat rows only; Teleport: pin rows only), so
        // they keep their original tight layout instead of reserving a checkbox-width slot.
        int checkboxAlign = (fld == &folders[F_INVENTORY]);
        int vp = 0;
        for (int i = 0; i < fld->count; ++i)
        {
            if (ItemHidden(folderIdx, &fld->items[i])) continue;
            if (vp >= scroll && vp < scroll + MAX_ROWS)
                DrawMenuItem(&fld->items[i], ROW_X, ROW_Y0 + (vp - scroll) * ROW_H, ROW_W, i == cursor, checkboxAlign);
            vp++;
        }
        int vis = vp;
        if (scroll > 0)
            for (int i = 0; i < 4; ++i) CFill(WIN_X + WIN_W - 14 - i, ROW_Y0 + 3 + i, 1 + 2 * i, 1, GOLD);
        if (scroll + MAX_ROWS < vis)
            for (int i = 0; i < 4; ++i) CFill(WIN_X + WIN_W - 14 - i, ROW_Y0 + MAX_ROWS * ROW_H - 4 - i, 1 + 2 * i, 1, GOLD);
    }

    (void)depth;
    CText6Btn(WIN_X + 12, WIN_Y + WIN_H - 16, T("{X} info   {Y} fav"), INK_DIM);
}

// Word-wrapped info overlay (Gen6-style "(i)" note). Any button closes it.
// Global "exit straight to the game" request. Any sub-loop (tool, picker,
// info box) sets this on SELECT; RunMenu sees it and unwinds all the way out
// instead of just backing up one level. The menu position is kept in the
// menu* statics, so the next SELECT reopens exactly where you left off.
static int g_quitToGame = 0;
// If we exited to the game from inside a tool, remember which one so the next
// SELECT jumps straight back into it (not the Tools folder). -1 = none.
static int g_resumeTool = -1;

static void InfoBox(const Item *it)
{
    // teleport rows carry their text in warps[]; everything else uses the Item's own label/desc
    const char *ibLabel = (it->warp >= 0) ? warps[it->warp].name : HkExpand(it->label, it->cheat);
    const char *ibDesc  = (it->warp >= 0) ? warps[it->warp].desc : it->desc;
    if (!ibDesc) return;

    int bw = 264, bx = WIN_X + (WIN_W - bw) / 2;
    char lines[8][64];
    int nlines = 0;

    // Live hotkey: a rebindable feature carries a {HK} token in its desc; swap it for the
    // currently-mapped glyph so the card always shows the real button.
    const char *s = HkExpand(T(ibDesc), it->cheat);
    while (*s && nlines < 8)
    {
        int len = 0;
        lines[nlines][0] = 0;
        while (*s)
        {
            while (*s == ' ') s++;
            if (*s == '\n') { s++; break; } // hard line break
            if (!*s) break;
            int wl = 0;
            while (s[wl] && s[wl] != ' ' && s[wl] != '\n') wl++;

            char tmp[64];
            int tl = len;
            memcpy(tmp, lines[nlines], (size_t)len);
            if (tl && tl < 62) tmp[tl++] = ' ';
            for (int k = 0; k < wl && tl < 63; ++k) tmp[tl++] = s[k];
            tmp[tl] = 0;

            if (len && CTextBtnWidth(tmp) > bw - 24) break; // word starts the next line (glyph-aware)
            memcpy(lines[nlines], tmp, (size_t)tl + 1);
            len = tl;
            s += wl;
        }
        nlines++;
    }

    int bh = 34 + nlines * 16 + 10;
    int by = WIN_Y + (WIN_H - bh) / 2;

    // The InfoBox is a fixed dark tooltip; use theme-independent light gold/ink so it stays
    // readable over EVERY theme (light themes' own INK/GOLD are dark and would vanish here).
    #define IB_GOLD 236, 200, 120
    #define IB_INK  248, 240, 216
    CFillInset(bx, by, bw, bh, 0);
    CFill(bx, by, bw, 1, IB_GOLD); CFill(bx, by + bh - 1, bw, 1, IB_GOLD);
    CFill(bx, by, 1, bh, IB_GOLD); CFill(bx + bw - 1, by, 1, bh, IB_GOLD);
    // title without any " (...)" tail - the description below already explains it
    char tit[48]; { const char *L = T(ibLabel); int k = 0;
        while (L[k] && k < 47 && !(L[k] == ' ' && L[k + 1] == '(')) { tit[k] = L[k]; k++; } tit[k] = 0; }
    CTextBtn(bx + 12, by + 6, tit, IB_GOLD, 1);
    CFill(bx + 12, by + 23, CTextBtnWidth(tit) + 6, 1, IB_GOLD);
    for (int i = 0; i < nlines; ++i)
        CTextBtn(bx + 12, by + 30 + i * 16, lines[i], IB_INK, 0);
    #undef IB_GOLD
    #undef IB_INK

    Present(); Present();

    // wait for any fresh key press, then release
    u32 prev = HID_PAD;
    while (1)
    {
        svcSleepThread(16 * 1000 * 1000);
        u32 pad = HID_PAD;
        if (pad & ~prev) { if (pad & BUTTON_SELECT) g_quitToGame = 1; break; }
        prev = pad;
    }
    DrainButtons(~0u);   // capped: a stuck pad must not hang the console with the game paused
}

// Picker grid UI (bottle contents, inventory item, forms). 2 columns, a real sprite/icon per
// option instead of a color swatch, no big value-preview card - just the list, like every other
// menu screen in the plugin. Returns after A (write) or B (cancel).
static void PickerRun(const Picker *pk)
{
    int cursor = 0, changed = 1;
    u8 cur = 0;
    // 0 = address not set or not mapped right now. When that happens we still show the grid
    // (so you can see the options), just with nothing marked as the current value.
    int haveCur = PickerRead(pk, &cur);
    if (haveCur)
        for (int k = 0; k < pk->count; ++k)
            if (pk->opts[k].val == cur) { cursor = k; break; }

    // Bottle (22 options) needs scrolling - 2 cols x MAX_ROWS(9) only fits 18. Everything else
    // (<= 18 options) never scrolls; `scroll` is a row offset, same convention as the rest of
    // the menu's list views.
    int cols = pk->count > 1 ? 2 : 1;
    int rows = (pk->count + cols - 1) / cols;
    int colW = ROW_W / cols;
    int scroll = 0;

    u32 prev = HID_PAD;

    while (1)
    {
        if (changed)
        {
            int curRow = cursor / cols;
            if (curRow < scroll) scroll = curRow;
            if (curRow >= scroll + MAX_ROWS) scroll = curRow - MAX_ROWS + 1;

            ComposeBackdrop();
            int tw = CTextWidth(T(pk->title));
            CText(WIN_X + 12, WIN_Y + 7, T(pk->title), GOLD, 1);
            CFill(WIN_X + 12, WIN_Y + 24, tw + 6, 1, GOLD);

            for (int i = 0; i < pk->count; ++i)
            {
                int col = i % cols, row = i / cols;
                if (row < scroll || row >= scroll + MAX_ROWS) continue;
                int x = ROW_X + col * colW, y = ROW_Y0 + (row - scroll) * ROW_H;
                int isCur = haveCur && pk->opts[i].val == cur;
                if (i == cursor)
                {
                    CFillBlend(x - 4, y - 1, colW - 4, ROW_H, 0, 0, 0, 110);
                    CFill(x - 4, y - 1, 2, ROW_H, GOLD);
                }
                if (pk->opts[i].icon >= 0) DrawSprite(x, y - 1, pk->opts[i].icon, 0);
                else { const u8 *sw = isCur ? CGREEN : CDIM; CFill(x + 2, y + 1, 12, 12, sw[0], sw[1], sw[2]); }
                const u8 *oc = isCur ? CGREEN : CINK;
                CTextClip(x + 20, y - 1, T(pk->opts[i].name), colW - 28, oc[0], oc[1], oc[2], 0);
                if (isCur) CFill(x + colW - 12, y + 3, 6, 6, CGREEN[0], CGREEN[1], CGREEN[2]); // "currently set" dot
            }
            if (scroll > 0)
                for (int a = 0; a < 4; ++a) CFill(WIN_X + WIN_W - 14 - a, ROW_Y0 + 3 + a, 1 + 2 * a, 1, GOLD);
            if (scroll + MAX_ROWS < rows)
                for (int a = 0; a < 4; ++a) CFill(WIN_X + WIN_W - 14 - a, ROW_Y0 + MAX_ROWS * ROW_H - 4 - a, 1 + 2 * a, 1, GOLD);

            CText6Btn(WIN_X + 12, WIN_Y + WIN_H - 16, T("{A} set   {B} cancel"), INK_DIM);
            Present(); Present();
            changed = 0;
        }

        svcSleepThread(16 * 1000 * 1000);
        u32 pad = HID_PAD, down = ARepeat(pad, &prev, &g_arHold);

        if (down & BUTTON_DOWN)
        {
            int col = cursor % cols, row = cursor / cols;
            for (int s = 1; s <= rows; ++s) { int nr = (row + s) % rows, idx = nr * cols + col; if (idx < pk->count) { cursor = idx; break; } }
            changed = 1;
        }
        if (down & BUTTON_UP)
        {
            int col = cursor % cols, row = cursor / cols;
            for (int s = 1; s <= rows; ++s) { int nr = (row - s + rows * 4) % rows, idx = nr * cols + col; if (idx < pk->count) { cursor = idx; break; } }
            changed = 1;
        }
        if (down & (BUTTON_RIGHT | BUTTON_LEFT | BUTTON_R1 | BUTTON_L1))
        {
            if (cols > 1)
            {
                int col = cursor % cols, row = cursor / cols, nc = 1 - col;
                int idx = row * cols + nc;
                if (idx < pk->count) cursor = idx;
            }
            changed = 1;
        }
        if (down & BUTTON_A)
        {
            if (PickerWrite(pk, pk->opts[cursor].val))
                QueueToastRaw(T(pk->opts[cursor].name), T(": SET"));
            else
                QueueToastRaw(T("Address not set"), ""); // placeholder / unmapped: refuse, don't crash
            break;
        }
        if (down & BUTTON_SELECT) g_quitToGame = 1;
        if (down & (BUTTON_B | BUTTON_SELECT)) break;
    }
}

// ===================== Tools =====================
// These run from inside RunMenu, so the game is already paused (memory stable).

// ---- Touch keypad (bottom screen). hid:USER is in the game's ACL, so unlike
// ir:rst we CAN read the touch panel. HID runs in its own process, so touch
// keeps updating even while the game threads are frozen. ----
// IMPORTANT: do NOT use libctru's hidInit() — on New 3DS it calls irrstInit()
// internally (hidShouldUseIrrst()), and ir:rst conflicts with the game and FREEZES
// the game (+ blocks the HOME button). We do a minimal manual hid:USER init:
// open the service, map the shared memory, read touch directly. No ir:rst.
static int   hidReady;
static vu32 *hidShmem;
static Handle g_hidMem;   // kept so PluginShutdown() can unmap the block again

static void KbInit(void)
{
    if (hidReady) return;
    mappableInit(OS_MAP_AREA_BEGIN, OS_MAP_AREA_END);

    Handle srv = 0;
    if (R_FAILED(srvGetServiceHandle(&srv, "hid:USER")) &&
        R_FAILED(srvGetServiceHandle(&srv, "hid:SPVR"))) return;

    u32 *cmd = getThreadCommandBuffer();
    cmd[0] = IPC_MakeHeader(0xA, 0, 0); // HIDUSER_GetIPCHandles
    Result r = svcSendSyncRequest(srv);
    if (R_SUCCEEDED(r)) r = (Result)cmd[1];
    Handle mem = 0, ev[5] = { 0 };
    if (R_SUCCEEDED(r)) { mem = cmd[3]; for (int i = 0; i < 5; ++i) ev[i] = cmd[4 + i]; }
    svcCloseHandle(srv);
    for (int i = 0; i < 5; ++i) if (ev[i]) svcCloseHandle(ev[i]); // events unused
    if (R_FAILED(r) || !mem) return;

    vu32 *sh = (vu32 *)mappableAlloc(0x2B0);
    if (!sh) { svcCloseHandle(mem); return; }
    if (R_FAILED(svcMapMemoryBlock(mem, (u32)sh, MEMPERM_READ, MEMPERM_DONTCARE)))
    { svcCloseHandle(mem); return; }

    hidShmem = sh;
    g_hidMem = mem;   // NOT closed here: svcUnmapMemoryBlock needs it at shutdown
    hidReady = 1;
}

// Read the touch panel directly from HID shared memory (layout per libctru:
// touch section at word 42, index at 42+4, entries {pos,valid} at 42+8+Id*2).
static int HidTouch(int *px, int *py)
{
    if (!hidShmem) return 0;
    u32 id = hidShmem[42 + 4]; if (id > 7) id = 7;
    u32 packed = hidShmem[42 + 8 + id * 2];
    u32 valid  = hidShmem[42 + 8 + id * 2 + 1];
    if (px) *px = (int)(packed & 0xFFFF);
    if (py) *py = (int)((packed >> 16) & 0xFFFF);
    return valid != 0;
}

// hit-test a touch against a key rect
static int KbHit(int tx, int ty, int x, int y, int w, int h)
{ return tx >= x && tx < x + w && ty >= y && ty < y + h; }

// OPTIONAL keypad skin. Leave NULL for the code-drawn keys. Point it at your own RGBA4444
// tile (any size - DrawScaled stretches it to each key rect) and the whole keypad is
// re-skinned without touching KbBuild/KbHit or a single line of input handling.
static const u16 *g_kbSkin = NULL;
static int g_kbSkinW = 0, g_kbSkinH = 0;

// One key, styled by kind. kinds: 0 num, 1 del/clr, 2 hex-on, 3 hex-off,
// 4 toggle, 5 OK, 6 Cancel.
//
// The keypad is CODE-DRAWN: every key is a themed rect plus centered text, so it follows the
// active theme for free and costs zero asset bytes. Crucially the ART IS INDEPENDENT OF THE
// LOGIC - KbBuild() owns the rects and KbHit() owns the hit-testing, so you can skin the
// keypad by pointing this function at DrawScaled() with your own RGBA4444 tiles and never
// touch a line of input handling.
static void KbKey(int x, int y, int w, int h, const char *label, int kind, int hot)
{
    // Key face: lift (or on a light theme, sink) the background so keys read as raised.
    int lift = ThemeBgLight() ? -26 : 28;
    int R = CBG[0] + lift, G = CBG[1] + lift, B = CBG[2] + lift;
    u8 tr = CINK[0], tg = CINK[1], tb = CINK[2];
    switch (kind)
    {
        case 3: // hex A-F while in DEC mode: disabled, so flatten it back toward the bg
            R = CBG[0] + lift / 3; G = CBG[1] + lift / 3; B = CBG[2] + lift / 3;
            tr = CDIM[0]; tg = CDIM[1]; tb = CDIM[2];
            break;
        case 4: tr = CGOLD[0];  tg = CGOLD[1];  tb = CGOLD[2];  break; // DEC/HEX toggle
        case 5: tr = CGREEN[0]; tg = CGREEN[1]; tb = CGREEN[2]; break; // OK
        case 6: tr = 240; tg = 150; tb = 130; break;                   // Cancel
        default: break;                                                // num / hex-on / Del / Clr
    }
    if (hot) { R += 36; G += 36; B += 36; }                            // press flash
    if (g_kbSkin)  // skinned: one source tile scaled onto this key, dimmed when disabled
        DrawScaled(x, y, w, h, g_kbSkin, g_kbSkinW, g_kbSkinH, kind == 3 ? 150 : 0);
    else
        CFill(x, y, w, h, ClampU8(R), ClampU8(G), ClampU8(B));
    CFill(x, y, w, 1, GOLD); CFill(x, y + h - 1, w, 1, GOLD);
    CFill(x, y, 1, h, GOLD); CFill(x + w - 1, y, 1, h, GOLD);
    int tw = CTextWidth(label);
    CText(x + (w - tw) / 2, y + (h - 15) / 2, label, tr, tg, tb, 1);
}

// Layout B: phone numpad + hex block + bottom OK/Cancel bar.
// codes: 0-15 = digit value; 20 Del; 21 Clr; 22 toggle; 23 OK; 24 Cancel
typedef struct { int x, y, w, h, code; const char *lab; } KbBtn;

static int KbBuild(KbBtn *b, int hex)
{
    // Plain rects on the 320x240 bottom screen: kw/kh = 52x30, pitch 57 x / 35 y. These rects
    // are the ONLY thing the touch handling knows about, which is why re-skinning KbKey()
    // cannot break input.
    int n = 0, kw = 52, kh = 30, XP = 57, YP = 35, x0 = 16, y0 = 53;
    static const int   pc[12] = { 1,2,3, 4,5,6, 7,8,9, 20,0,21 };
    static const char *pl[12] = { "1","2","3","4","5","6","7","8","9","Del","0","Clr" };
    for (int i = 0; i < 12; ++i)
    { int c = i % 3, r = i / 3;
      b[n++] = (KbBtn){ x0 + c*XP, y0 + r*YP, kw, kh, pc[i], pl[i] }; }

    int hx = 196;
    static const int   hc[6] = { 10,11,12,13,14,15 };
    static const char *hl[6] = { "A","B","C","D","E","F" };
    for (int i = 0; i < 6; ++i)
    { int c = i % 2, r = i / 2;
      b[n++] = (KbBtn){ hx + c*XP, y0 + r*YP, kw, kh, hc[i], hl[i] }; }

    b[n++] = (KbBtn){ hx, 158, 109, 30, 22, hex ? "HEX" : "DEC" };
    b[n++] = (KbBtn){ 51,  198, 96, 32, 23, "OK" };
    b[n++] = (KbBtn){ 173, 198, 96, 32, 24, "Cancel" };
    return n;
}

// Touch keypad entry (decimal, with HEX toggle). Returns value; *cancel on Cancel.
static u32 EnterNum(const char *title, u32 initial, int *cancel)
{
    KbInit();
    *cancel = 0;
    u32 val = initial;
    int hex = 0, changed = 1, hotCode = -1;
    int touchPrev = HidTouch(0, 0); // ignore a touch already held on entry

    while (1)
    {
        KbBtn b[24];
        int nb = KbBuild(b, hex);

        if (changed)
        {
            // dim frozen bottom frame as backdrop
            for (int y = 0; y < BOT_H; ++y)
                for (int x = 0; x < BOT_W; ++x)
                {
                    u8 *p = CPix(x, y);
                    if (savedBotValid)
                    { u16 v = savedBot[y * BOT_W + x];
                      p[0] = (u8)((((v>>11)&31)<<3)/3); p[1] = (u8)((((v>>5)&63)<<2)/3); p[2] = (u8)(((v&31)<<3)/3); }
                    else { p[0] = p[1] = p[2] = 12; }
                }
            CText(14, 8, title, GOLD, 1);
            char disp[40];
            if (hex) siprintf(disp, "0x%lX", (unsigned long)val);
            else     siprintf(disp, "%lu  (0x%lX)", (unsigned long)val, (unsigned long)val);
            CFillInset(12, 30, 296, 18, 0);
            CText(18, 31, disp, INK, 0);

            for (int i = 0; i < nb; ++i)
            {
                int c = b[i].code;
                int kind;
                if      (c <= 9)             kind = 0;           // digits
                else if (c <= 15)            kind = hex ? 2 : 3; // hex A-F, greyed out in DEC mode
                else if (c == 20 || c == 21) kind = 1;           // Del / Clr
                else if (c == 22)            kind = 4;           // DEC/HEX toggle
                else if (c == 23)            kind = 5;           // OK
                else                         kind = 6;           // Cancel
                // The toggle is excluded from the press flash: swapping DEC/HEX is feedback
                // enough, and the flash would stay lit until the next key.
                KbKey(b[i].x, b[i].y, b[i].w, b[i].h, b[i].lab, kind, (hotCode == c) && c != 22);
            }
            BotBlitComposeBoth();
            changed = 0;
        }

        svcSleepThread(16 * 1000 * 1000);
        if (HID_PAD & BUTTON_B) { *cancel = 1; return initial; } // physical B cancels

        int px, py, now = HidTouch(&px, &py);
        int tap = now && !touchPrev; touchPrev = now;
        if (!hidReady || !tap) continue;

        for (int i = 0; i < nb; ++i)
        {
            if (!KbHit(px, py, b[i].x, b[i].y, b[i].w, b[i].h)) continue;
            int c = b[i].code;
            if (c <= 15)
            {
                if (c >= 10 && !hex) break;             // hex digit disabled in dec
                u32 base = hex ? 16 : 10, nv = val * base + c;
                if (nv >= val) val = nv;                 // ignore overflow
                hotCode = c;
            }
            else if (c == 20) { val = hex ? (val >> 4) : (val / 10); hotCode = c; }
            else if (c == 21) { val = 0; hotCode = c; }
            else if (c == 22) { hex = !hex; hotCode = c; }
            else if (c == 23) return val;
            else { *cancel = 1; return initial; }
            changed = 1;
            break;
        }
    }
}

// ---- memory region walk (game's own RW private regions) ----
// Region clamp for the walk (set from the Cheat Search "Memory Region" preset).
static u32 g_scanLo, g_scanHi;
typedef void (*RegionCb)(u32 base, u32 size, void *ud);
static void ForEachRWRegion(RegionCb cb, void *ud)
{
    u32 lo = g_scanLo ? g_scanLo : 0x00100000;
    u32 hi = g_scanHi ? g_scanHi : 0x40000000;
    u32 addr = lo;
    while (addr < hi)
    {
        MemInfo info; PageInfo pg;
        if (R_FAILED(svcQueryMemory(&info, &pg, addr))) break;
        u32 next = info.base_addr + info.size;
        if (info.size == 0) break;
        // any readable+writable region (skip free/IO/reserved). This covers the
        // game's .data/.bss and heaps, whatever their exact MemState. Read-only
        // shared blocks (font, hid shmem) are excluded by the WRITE requirement.
        if ((info.perm & MEMPERM_READ) && (info.perm & MEMPERM_WRITE) &&
            info.state != MEMSTATE_FREE && info.state != MEMSTATE_IO &&
            info.state != MEMSTATE_RESERVED)
        {
            u32 b = info.base_addr, e = info.base_addr + info.size;
            if (b < lo) b = lo;   // clamp to the selected region window
            if (e > hi) e = hi;
            if (e > b) cb(b, e - b, ud);
        }
        if (next <= addr) break;
        addr = next;
    }
}

static u32 ReadN(u32 a, int w)
{
    if (w == 1) return R8(a);
    if (w == 2) return R16(a);
    return R32(a);
}
static void WriteN(u32 a, int w, u32 v)
{
    if (w == 1) W8(a, (u8)v);
    else if (w == 2) W16(a, (u16)v);
    else W32(a, v);
}

// ---- Cheat Search (CTRPF-style: results table on top, form on bottom) ----
typedef struct { u32 addr, newv, oldv; } Cand;
static Cand *g_cands;
static u32   g_candCap, g_candCount;
static int   g_searchStarted, g_searchWidth = 4; // bytes: 1/2/4
static int   g_scanType = 0;   // 0 =  1 >  2 <  3 changed  4 unchanged  5 increased  6 decreased
static int   g_step, g_capped;
static u32   g_searchValue;
static u32   g_searchSelAddr;  // address under the cursor (shared with RAM Dumper)
static int   g_searchType = 0; // 0 = Known Value, 1 = Unknown Search
static int   g_memRegion  = 0; // region preset index

// Unknown Search: raw snapshot of the scanned region(s). We compare live memory
// against this later, so a value we never knew can still be tracked by how it
// changes. Bytes (not per-address candidates) keeps it compact enough to fit.
#define SNAP_CAP (2u * 1024 * 1024)
static u8   *g_snap;             // lazily allocated, persists for the session
static u32   g_snapUsed;
typedef struct { u32 base, size, off; } SnapReg;
static SnapReg g_snapReg[64];
static int   g_snapRegN;
static int   g_unknownArmed;     // snapshot taken, awaiting first comparison scan

// One-step Undo: backup of the candidate list before the last scan.
static Cand *g_undo;
static u32   g_undoCount;
static int   g_undoStep, g_undoArmed, g_undoStarted, g_undoValid;

static const char *SCAN_NAME[7] = {
    "Equal To", "Greater Than", "Less Than", "Changed", "Unchanged", "Increased", "Decreased"
};
static const char *SEARCHTYPE_NAME[2] = { "Known Value", "Unknown Search" };
#define NUM_REGIONS 4
static const char *REGION_NAME[NUM_REGIONS] = {
    "All Memory", "Low 1-128M", "Mid 128-512M", "High 512M-1G"
};
static void RegionBounds(int r, u32 *lo, u32 *hi)
{
    switch (r) {
        case 1: *lo = 0x00100000; *hi = 0x08000000; break;
        case 2: *lo = 0x08000000; *hi = 0x20000000; break;
        case 3: *lo = 0x20000000; *hi = 0x40000000; break;
        default:*lo = 0x00100000; *hi = 0x40000000; break;
    }
}
static int ScanNeedsValue(int st) { return st <= 2; } // 0/1/2 use the value field

static u32 g_dbgRegions, g_dbgKB; // diagnostics: what the last scan visited

typedef struct { u32 value; } SeedCtx;
static void SeedCb(u32 base, u32 size, void *ud)
{
    g_dbgRegions++; g_dbgKB += size / 1024;
    u32 v = ((SeedCtx *)ud)->value; int w = g_searchWidth;
    for (u32 a = base; a + w <= base + size; a += w)
    {
        if (ReadN(a, w) != v) continue;
        if (g_candCount >= g_candCap) { g_capped = 1; return; }
        g_cands[g_candCount].addr = a;
        g_cands[g_candCount].newv = v;
        g_cands[g_candCount].oldv = v;
        g_candCount++;
    }
}
static void SearchSeed(u32 v)
{
    g_candCount = 0; g_capped = 0; g_dbgRegions = 0; g_dbgKB = 0;
    SeedCtx c = { v };
    ForEachRWRegion(SeedCb, &c);
    g_searchStarted = 1; g_step = 1;
}
static int MatchScan(int st, u32 nv, u32 ov, u32 val)
{
    switch (st)
    {
        case 0: return nv == val;  case 1: return nv > val;  case 2: return nv < val;
        case 3: return nv != ov;   case 4: return nv == ov;
        case 5: return nv > ov;    default: return nv < ov;
    }
}
static void SearchNext(u32 val)
{
    int w = g_searchWidth; u32 keep = 0;
    for (u32 i = 0; i < g_candCount; ++i)
    {
        u32 nv = ReadN(g_cands[i].addr, w), ov = g_cands[i].newv;
        if (MatchScan(g_scanType, nv, ov, val))
        {
            g_cands[keep].addr = g_cands[i].addr;
            g_cands[keep].newv = nv; g_cands[keep].oldv = ov; keep++;
        }
    }
    g_candCount = keep; g_step++;
}

// ---- Unknown Search: raw snapshot, compared on the next scan ----
static void SnapCb(u32 base, u32 size, void *ud)
{
    (void)ud;
    if (!g_snap || g_snapRegN >= 64 || g_snapUsed >= SNAP_CAP) { g_capped = 1; return; }
    u32 room = SNAP_CAP - g_snapUsed;
    u32 n = size; if (n > room) { n = room; g_capped = 1; }
    memcpy(g_snap + g_snapUsed, (const void *)base, n);
    g_snapReg[g_snapRegN].base = base;
    g_snapReg[g_snapRegN].size = n;
    g_snapReg[g_snapRegN].off  = g_snapUsed;
    g_snapRegN++;
    g_snapUsed += n;
}
static void SnapshotArm(void)
{
    g_candCount = 0; g_capped = 0; g_snapUsed = 0; g_snapRegN = 0;
    if (!g_snap) { QueueToastRaw("No snapshot buffer", ""); return; }
    ForEachRWRegion(SnapCb, NULL);
    g_searchStarted = 1; g_unknownArmed = 1; g_step = 1;
}
static u32 SnapN(u32 off, int w)
{
    u8 *p = g_snap + off;
    if (w == 1) return p[0];
    if (w == 2) return (u32)p[0] | ((u32)p[1] << 8);
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}
// First comparison after a snapshot: build candidates where live memory relates
// to the snapshot the way the scan type says (Changed / Increased / Decreased...).
static void SnapshotMaterialize(u32 val)
{
    int w = g_searchWidth; g_candCount = 0; g_capped = 0;
    for (int r = 0; r < g_snapRegN; ++r)
    {
        u32 base = g_snapReg[r].base, sz = g_snapReg[r].size, off = g_snapReg[r].off;
        for (u32 a = base; a + w <= base + sz; a += w)
        {
            u32 nv = ReadN(a, w);
            u32 ov = SnapN(off + (a - base), w);
            if (!MatchScan(g_scanType, nv, ov, val)) continue;
            if (g_candCount >= g_candCap) { g_capped = 1; break; }
            g_cands[g_candCount].addr = a;
            g_cands[g_candCount].newv = nv;
            g_cands[g_candCount].oldv = ov;
            g_candCount++;
        }
        if (g_capped) break;
    }
    g_unknownArmed = 0; g_step++;
}

// ---- one-step Undo ----
static void SaveUndo(void)
{
    if (!g_undo) return;
    memcpy(g_undo, g_cands, g_candCount * sizeof(Cand));
    g_undoCount = g_candCount; g_undoStep = g_step;
    g_undoArmed = g_unknownArmed; g_undoStarted = g_searchStarted;
    g_undoValid = 1;
}
static void DoUndo(void)
{
    if (!g_undoValid || !g_undo) { QueueToastRaw("Nothing to undo", ""); return; }
    memcpy(g_cands, g_undo, g_undoCount * sizeof(Cand));
    g_candCount = g_undoCount; g_step = g_undoStep;
    g_unknownArmed = g_undoArmed; g_searchStarted = g_undoStarted;
    g_undoValid = 0; g_capped = 0;
    QueueToastRaw("Undo", ": last scan reverted");
}

// perform a search step using the current form state
static void DoSearch(void)
{
    if (!g_candCap) { QueueToastRaw("No memory", ""); return; }
    RegionBounds(g_memRegion, &g_scanLo, &g_scanHi); // apply the region window
    SaveUndo();
    if (!g_searchStarted)
    {
        if (g_searchType == 1) SnapshotArm();            // Unknown: capture snapshot
        else                   SearchSeed(g_searchValue);// Known: seed by value
    }
    else if (g_unknownArmed) SnapshotMaterialize(g_searchValue); // first compare vs snapshot
    else                     SearchNext(g_searchValue);          // subsequent filters
}

// --- top screen: results table (Address / New / Old) with a selection cursor ---
#define SR_ROWS 9   // visible result rows
static void SearchDrawResults(int scroll, int cursor)
{
    ComposeBackdrop();
    CText(WIN_X + 12, WIN_Y + 6, T("Cheat Search"), GOLD, 1);
    char hit[48];
    if (!g_searchStarted) sniprintf(hit, sizeof hit, "%s", T("no search"));
    else if (g_unknownArmed) sniprintf(hit, sizeof hit, "Snapshot %luKB%s", (unsigned long)(g_snapUsed / 1024), g_capped ? "+" : "");
    else sniprintf(hit, sizeof hit, "Step %d   Hits: %lu%s", g_step, (unsigned long)g_candCount, g_capped ? "+" : "");
    CText6(WIN_X + WIN_W - 12 - C6Width(hit), WIN_Y + 9, hit, g_capped ? 233 : 196, g_capped ? 115 : 180, g_capped ? 107 : 150);
    CFill(WIN_X + 12, WIN_Y + 22, WIN_W - 24, 1, GOLD);

    int cx0 = WIN_X + 16, cx1 = WIN_X + 140, cx2 = WIN_X + 244;
    CText6(cx0, WIN_Y + 26, T("Address"), INK_DIM);
    CText6(cx1, WIN_Y + 26, T("New Value"), INK_DIM);
    CText6(cx2, WIN_Y + 26, T("Old Value"), INK_DIM);

    int rowY = WIN_Y + 40, rh = 15, w = g_searchWidth;
    for (int i = scroll; i < (int)g_candCount && i < scroll + SR_ROWS; ++i)
    {
        int y = rowY + (i - scroll) * rh;
        if (i == cursor)
        {
            CFillBlend(WIN_X + 12, y - 1, WIN_W - 24, rh, 0, 0, 0, 120);
            CFill(WIN_X + 12, y - 1, 2, rh, GOLD);
        }
        char a[12], nv[14], ov[14];
        siprintf(a, "%08lX", (unsigned long)g_cands[i].addr);
        siprintf(nv, "%lu", (unsigned long)ReadN(g_cands[i].addr, w)); // New = live value
        siprintf(ov, "%lu", (unsigned long)g_cands[i].newv);           // Old = value at last scan (the Next baseline)
        CText6(cx0, y, a, (i == cursor) ? 246 : 236, (i == cursor) ? 236 : 224, (i == cursor) ? 200 : 198);
        CText6(cx1, y, nv, GREEN_ON);
        CText6(cx2, y, ov, INK_DIM);
    }
    if (!g_candCount)
    {
        if (g_unknownArmed)
        {
            CText6(cx0, rowY,      T("Snapshot taken."), GREEN_ON);
            CText6(cx0, rowY + 16, T("1. Change the value in the game."), INK_DIM);
            CText6(cx0, rowY + 29, T("2. Pick Changed / Decreased / etc."), INK_DIM);
            CText6(cx0, rowY + 42, T("3. Search. Repeat to narrow down."), INK_DIM);
        }
        else
            CText6(cx0, rowY, g_searchStarted ? T("(no matches)") : T("Set the form below, then Search."), INK_DIM);
    }

    // scroll arrows (right edge)
    if (scroll > 0)
        for (int a = 0; a < 4; ++a) CFill(WIN_X + WIN_W - 16 - a, rowY + 2 + a, 1 + 2 * a, 1, GOLD);
    if (scroll + SR_ROWS < (int)g_candCount)
        for (int a = 0; a < 4; ++a) CFill(WIN_X + WIN_W - 16 - a, rowY + SR_ROWS * rh - 4 - a, 1 + 2 * a, 1, GOLD);

    // footer: controls (left) + page indicator (right), guarded against overlap
    if (g_candCount)
    {
        const char *leg = T("{A} poke  {DP} move  {B} exit");
        CText6(WIN_X + 12, WIN_Y + WIN_H - 14, leg, INK_DIM);
        char pg[28];
        siprintf(pg, "%d / %lu", cursor + 1, (unsigned long)g_candCount);
        int pgX = WIN_X + WIN_W - 12 - C6Width(pg);
        int legEnd = WIN_X + 12 + C6Width(leg);
        if (pgX > legEnd + 8) // only draw if it clears the legend
            CText6(pgX, WIN_Y + WIN_H - 14, pg, INK_DIM);
    }
    else
        CText6Btn(WIN_X + 12, WIN_Y + WIN_H - 14, T("{X} type  {Y} scan  {R} search  {B} exit"), INK_DIM);
}

// --- bottom screen: touch form ---
// codes: 1 valtype  2 scan  3 value  4 search  5 reset  6 region  7 searchtype  8 undo
typedef struct { int x, y, w, h, code; } FBox;
#define SF_VX  120
#define SF_VW  186
#define SF_FY  36
#define SF_FH  21
#define SF_G   4
static int SearchBuildForm(FBox *f)
{
    int n = 0;
    static const int codes[5] = { 6, 7, 1, 2, 3 }; // region, search type, value type, scan, value
    for (int i = 0; i < 5; ++i)
        f[n++] = (FBox){ SF_VX, SF_FY + i * (SF_FH + SF_G), SF_VW, SF_FH, codes[i] };
    int by = SF_FY + 5 * (SF_FH + SF_G) + 8;
    f[n++] = (FBox){ 14,  by, 93, 28, 4 };  // Search
    f[n++] = (FBox){ 113, by, 93, 28, 8 };  // Undo
    f[n++] = (FBox){ 212, by, 94, 28, 5 };  // Reset
    return n;
}
static void SearchDrawForm(void)
{
    // raw frozen game frame...
    for (int y = 0; y < BOT_H; ++y)
        for (int x = 0; x < BOT_W; ++x)
        {
            u8 *p = CPix(x, y);
            if (savedBotValid)
            { u16 v = savedBot[y * BOT_W + x];
              p[0] = (u8)(((v>>11)&31)<<3); p[1] = (u8)(((v>>5)&63)<<2); p[2] = (u8)((v&31)<<3); }
            else { p[0] = p[1] = p[2] = 12; }
        }
    // ...covered by a mostly-opaque themed panel so fields are always readable
    CFillBlend(0, 0, BOT_W, BOT_H, BG, 230); // ~90% opaque
    CFill(6, 4, BOT_W - 12, 1, GOLD); CFill(6, BOT_H - 6, BOT_W - 12, 1, GOLD);

    CText(14, 8, T("Cheat Search"), GOLD, 1);
    CFill(14, 30, C6Width(T("Cheat Search")) * 2, 1, GOLD);

    int lx = 14, vx = SF_VX, vw = SF_VW, fy = SF_FY, fh = SF_FH, g = SF_G;
    const char *labels[5] = { "Memory Region", "Search Type", "Value Type", "Scan Type", "Value" };
    int locked = g_searchStarted; // region / type / width are fixed once a search starts
    int dimValue = (!ScanNeedsValue(g_scanType)) || (g_searchType == 1 && !g_searchStarted);
    int dims[5] = { locked, locked, locked, 0, dimValue };
    char val[5][40];
    sniprintf(val[0], sizeof val[0], "%s", T(REGION_NAME[g_memRegion]));
    sniprintf(val[1], sizeof val[1], "%s", T(SEARCHTYPE_NAME[g_searchType]));
    sniprintf(val[2], sizeof val[2], "%d Bytes  (%d-bit)", g_searchWidth, g_searchWidth * 8);
    sniprintf(val[3], sizeof val[3], "%s", T(SCAN_NAME[g_scanType]));
    if (g_searchType == 1 && !g_searchStarted) sniprintf(val[4], sizeof val[4], "%s", T("(not needed)"));
    else if (dimValue)                         sniprintf(val[4], sizeof val[4], "--");
    else sniprintf(val[4], sizeof val[4], "%lu  (0x%lX)", (unsigned long)g_searchValue, (unsigned long)g_searchValue);

    for (int i = 0; i < 5; ++i)
    {
        int y = fy + i * (fh + g);
        CText6(lx, y + 4, T(labels[i]), INK);
        int dim = dims[i];
        // Field face derived from the THEME, same lift trick as the keypad keys: raise the
        // background (or sink it on a light theme) so the box reads as an input, and flatten it
        // back toward the background when the field is locked. These were hardcoded olive
        // values left over from the original palette, which ignored the theme entirely.
        CFillInset(vx, y, vw, fh, dim);
        CFill(vx, y, vw, 1, GOLD); CFill(vx, y + fh - 1, vw, 1, GOLD);
        CFill(vx, y, 1, fh, GOLD); CFill(vx + vw - 1, y, 1, fh, GOLD);
        const u8 *fc = dim ? CDIM : CINK;
        CText6(vx + 6, y + 4, val[i], fc[0], fc[1], fc[2]);
    }
    int by = fy + 5 * (fh + g) + 8;
    KbKey(14,  by, 93, 28, g_searchStarted ? T("Next") : T("Search"), 5, 0);
    KbKey(113, by, 93, 28, T("Undo"), g_undoValid ? 4 : 3, 0);
    KbKey(212, by, 94, 28, T("Reset"), 6, 0);
    const char *lg1 = T("Tap a field, or:");
    const char *lg2 = T("{X} type   {Y} scan   {R} search   {L} undo");
    CText6((BOT_W - C6Width(lg1)) / 2, by + 31, lg1, INK_DIM);
    CText6Btn((BOT_W - C6BtnWidth(lg2)) / 2, by + 43, lg2, INK_DIM);
    BotBlitComposeBoth();
}

static void ToolSearch(void)
{
    KbInit(); // touch input for the form (was missing -> taps did nothing)
    if (!g_cands)
    {
        g_candCap = 0x8000; // 32768 * 12B = 384KB
        g_cands = (Cand *)malloc(g_candCap * sizeof(Cand));
        if (!g_cands) g_candCap = 0;
    }
    if (!g_undo && g_candCap)                       // one-step undo backup (384KB)
        g_undo = (Cand *)malloc(g_candCap * sizeof(Cand));
    if (!g_snap)                                    // Unknown Search snapshot (2MB)
        g_snap = (u8 *)malloc(SNAP_CAP);

    int scroll = 0, cursor = 0, redrawTop = 1, redrawBot = 1;
    u32 prev = HID_PAD;
    int touchPrev = HidTouch(0, 0);

    while (1)
    {
        if (cursor >= (int)g_candCount) cursor = g_candCount ? g_candCount - 1 : 0;
        if (cursor < scroll) scroll = cursor;
        if (cursor >= scroll + SR_ROWS) scroll = cursor - SR_ROWS + 1;
        if (g_candCount) g_searchSelAddr = g_cands[cursor].addr; // shared with RAM Dumper

        if (redrawTop) { SearchDrawResults(scroll, cursor); Present(); Present(); redrawTop = 0; }
        if (redrawBot) { SearchDrawForm(); redrawBot = 0; }

        svcSleepThread(16 * 1000 * 1000);
        u32 pad = HID_PAD, down = ARepeat(pad, &prev, &g_arHold);

        if ((down & BUTTON_DOWN) && cursor + 1 < (int)g_candCount) { cursor++; redrawTop = 1; }
        if ((down & BUTTON_UP)   && cursor > 0)                    { cursor--; redrawTop = 1; }
        if (down & BUTTON_B) break;
        if (down & BUTTON_SELECT) { g_quitToGame = 1; break; } // straight to game, results kept

        // physical shortcuts: X = value type, Y = scan type, R = Search
        if (down & BUTTON_X)
        {
            if (!g_searchStarted) g_searchWidth = (g_searchWidth == 1) ? 2 : (g_searchWidth == 2) ? 4 : 1;
            else QueueToastRaw("Reset to change width", "");
            redrawTop = 1; redrawBot = 1;
        }
        if (down & BUTTON_Y)
        {
            g_scanType = (g_scanType + 1) % 7;
            redrawTop = 1; redrawBot = 1;
        }
        if (down & BUTTON_L1) // L = undo last scan
        {
            DoUndo(); scroll = 0; cursor = 0;
            redrawTop = 1; redrawBot = 1;
        }
        if (down & BUTTON_R1)
        {
            ComposeBackdrop(); CText(WIN_X + 110, WIN_Y + 90, T("Scanning..."), GOLD, 1); Present(); Present();
            DoSearch(); scroll = 0; cursor = 0;
            redrawTop = 1; redrawBot = 1;
            prev = HID_PAD;
            continue;
        }

        // A = poke the selected result
        if ((down & BUTTON_A) && g_candCount)
        {
            int cancel;
            u32 addr = g_cands[cursor].addr;
            u32 nv = EnterNum("Poke value", ReadN(addr, g_searchWidth), &cancel);
            if (!cancel) { WriteN(addr, g_searchWidth, nv); g_cands[cursor].newv = nv; QueueToastRaw("Poked", ""); }
            redrawTop = 1; redrawBot = 1;
            touchPrev = HidTouch(0, 0);
            continue;
        }

        int px, py, now = HidTouch(&px, &py);
        int tap = now && !touchPrev; touchPrev = now;
        if (!hidReady || !tap) continue;
        FBox f[8]; int nf = SearchBuildForm(f);
        for (int i = 0; i < nf; ++i)
        {
            if (!KbHit(px, py, f[i].x, f[i].y, f[i].w, f[i].h)) continue;
            switch (f[i].code)
            {
                case 1: // value type (only before first search)
                    if (!g_searchStarted)
                        g_searchWidth = (g_searchWidth == 1) ? 2 : (g_searchWidth == 2) ? 4 : 1;
                    else QueueToastRaw("Reset to change width", "");
                    break;
                case 2: g_scanType = (g_scanType + 1) % 7; break;
                case 3: // value entry
                {
                    int cancel;
                    u32 v = EnterNum("Search value", g_searchValue, &cancel);
                    if (!cancel) g_searchValue = v;
                    break;
                }
                case 4: // Search
                    ComposeBackdrop(); CText(WIN_X + 110, WIN_Y + 90, T("Scanning..."), GOLD, 1); Present(); Present();
                    DoSearch(); scroll = 0; cursor = 0;
                    break;
                case 5: // Reset (clears results, snapshot and undo; keeps your form choices)
                    g_candCount = 0; g_searchStarted = 0; g_capped = 0; g_step = 0;
                    g_unknownArmed = 0; g_snapUsed = 0; g_snapRegN = 0; g_undoValid = 0;
                    scroll = 0; cursor = 0;
                    break;
                case 6: // memory region (only before a search starts)
                    if (!g_searchStarted) g_memRegion = (g_memRegion + 1) % NUM_REGIONS;
                    else QueueToastRaw("Reset to change region", "");
                    break;
                case 7: // search type: Known / Unknown (only before a search starts)
                    if (!g_searchStarted) g_searchType = (g_searchType + 1) % 2;
                    else QueueToastRaw("Reset to change type", "");
                    break;
                case 8: // Undo
                    DoUndo(); scroll = 0; cursor = 0;
                    break;
            }
            redrawTop = 1; redrawBot = 1;
            touchPrev = HidTouch(0, 0);
            break;
        }
    }

    // Search state PERSISTS across tool exits and full menu close/reopen.
    // This is what enables the real cheat-search loop: seed a value -> exit menu
    // (unpauses the game) -> change the value in-game -> reopen -> filter by
    // Changed / Increased / Decreased / equals. The g_cands buffer stays
    // allocated for the plugin lifetime; only the "Reset" button clears results.
}

// ---- About (scrollable credits) ----
static void ToolAbout(void)
{
    static const struct { const char *s; int gold; } lines[] = {
        // EDIT ME - this is your plugin's credits screen. It is deliberately TEXT ONLY:
        // the blank template ships no logo image, so there is no third-party art to inherit.
        //
        // Replace the "Your plugin" block with your own name and repo. Please KEEP the
        // "CTRComposer engine" block - that is the attribution for the engine you are
        // building on, the same way the engine credits Luma3DS and PabloMK7 below it.
#if TOOLS_ONLY
        { PLUGIN_NAME,                   1 },
        { PLUGIN_VER,                    0 },
        { "",                            0 },
        { "A universal memory toolkit:",            0 },
        { "Cheat Search, RAM Dumper and a",         0 },
        { "Hex Editor, in every title.",            0 },
        { "",                            0 },
        { "Installed as default.3gx, so it",        0 },
        { "loads wherever no other plugin does.",   0 },
        { "",                            0 },
        { "It carries no cheats on purpose -",      0 },
        { "an address belongs to one game and",     0 },
        { "one region. Build your own plugin",      0 },
        { "from the CTRComposer template.",         0 },
        { "",                            0 },
#else
        { PLUGIN_NAME,                   1 },
        { PLUGIN_VER,                    0 },
        { "",                            0 },
        { "Majora's Mask 3D cheats + tools",        0 },
        { "Made by samaBR",              1 },
        { "github.com/samaBR85/MajoraCTRComposer",  0 },
        { "",                            0 },
#endif
        { "CTRComposer engine",          1 },
        { "A raw .3gx overlay engine for",          0 },
        { "the Nintendo 3DS.",                      0 },
        { "Made by samaBR",              1 },
        { "github.com/samaBR85/CTRComposer",        0 },
        { "",                            0 },
        { "Engine credits",              1 },
        { "Inspired by CTRPluginFramework",         0 },
        { "Loader: Luma3DS (LumaTeam)",  0 },
        { "3GX loader / tool: PabloMK7", 0 },
        { "Small font: Linux 6x10 console font",    0 },
        { "",                            0 },
#if !TOOLS_ONLY
        { "MM3D cheat table credits",    1 },
        { "Cheat list: JourneyOver",     0 },
        { "(CTRPF-AR-CHEAT-CODES)",      0 },
        { "Save-file mapping: HTW",      0 },
        { "(HelpTheWretched)",           0 },
        { "",                            0 },
        { "Icon credits",                1 },
        { "Item Icons: Colbydude",       0 },
        { "UI (rupee): xAct",            0 },
        { "Logo: Hiccup / Cheesy Mac",   0 },
        { "  n Cheese",                  0 },
        { "Buttons: manpaint",           0 },
        { "The Spriters Resource",       0 },
        { "",                            0 },
        { "Termina theme art: samaBR",   1 },
        { "(original artwork)",          0 },
#endif
    };
    int N = (int)(sizeof(lines) / sizeof(lines[0]));
    int x = WIN_X + 16;
#if TOOLS_ONLY
    int top = WIN_Y + 14;                 // text block starts at the top: no logo image
#else
    int top = WIN_Y + 10 + LOGO_H + 10;   // text block starts below the MM3D title logo
#endif
    int footY = WIN_Y + WIN_H - 16;
    int vis = (footY - top - 4) / 12;      // lines that fit in the scroll area
    int scroll = 0, redraw = 1;
    u32 prev = HID_PAD;
    while (1)
    {
        if (redraw)
        {
            ComposeBackdrop();
#if !TOOLS_ONLY
            DrawImg(WIN_X + (WIN_W - LOGO_W) / 2, WIN_Y + 10, logoPx, LOGO_W, LOGO_H);
#endif
            for (int i = 0; i < vis && scroll + i < N; ++i)
            {
                const char *s = lines[scroll + i].s;
                const u8 *lc = lines[scroll + i].gold ? CGOLD : CDIM;
                if (s[0]) CText6(x, top + i * 12, s, lc[0], lc[1], lc[2]);
            }
            if (scroll > 0)                          // up arrow (more above)
                for (int a = 0; a < 4; ++a) CFill(WIN_X + WIN_W - 16 - a, top + 3 + a, 1 + 2*a, 1, GOLD);
            if (scroll + vis < N)                    // down arrow (more below)
                for (int a = 0; a < 4; ++a) CFill(WIN_X + WIN_W - 16 - a, footY - 6 - a, 1 + 2*a, 1, GOLD);
            CText6Btn(x, footY, T("{DP} scroll    {B} back"), INK_DIM);
            Present(); Present();
            redraw = 0;
        }
        svcSleepThread(16 * 1000 * 1000);
        u32 pad = HID_PAD, down = ARepeat(pad, &prev, &g_arHold);
        if ((down & BUTTON_DOWN) && scroll + vis < N) { scroll++; redraw = 1; }
        if ((down & BUTTON_UP)   && scroll > 0)       { scroll--; redraw = 1; }
        if (down & BUTTON_SELECT) { g_quitToGame = 1; break; }
        if (down & (BUTTON_B | BUTTON_A)) break;
    }
    DrainButtons(~0u);   // capped: a stuck pad must not hang the console with the game paused
}

// ---- RAM Dumper ----
static u32 g_dumpStart = 0x08000000;
static int g_dumpSizeIdx = 3;
static const u32   DUMP_SIZES[6]   = { 0x1000, 0x10000, 0x40000, 0x100000, 0x400000, 0x1000000 };
static const char *DUMP_SIZE_NM[6] = { "4 KB", "64 KB", "256 KB", "1 MB", "4 MB", "16 MB" };
#define NUM_DUMP_SIZES 6
#define DUMP_LEAF "dumps"   // created under the plugin's own folder (see PlgPath)

// How many contiguous readable bytes exist from `start`, so a dump never faults
// on an unmapped hole. Returns bytes readable (<= want), or 0 if start is dead.
static u32 ReadableSpan(u32 start, u32 want)
{
    u32 end = start + want, a = start;
    while (a < end)
    {
        MemInfo info; PageInfo pg;
        if (R_FAILED(svcQueryMemory(&info, &pg, a))) break;
        u32 rend = info.base_addr + info.size;
        if (!(info.perm & MEMPERM_READ) || info.state == MEMSTATE_FREE) break;
        if (rend >= end) return want;
        if (rend <= a) break;
        a = rend;
    }
    return a > start ? a - start : 0;
}

static void RamDumpDrawTop(const char *status, u8 sr, u8 sg, u8 sb, int pct)
{
    ComposeBackdrop();
    CText(WIN_X + 12, WIN_Y + 6, T("RAM Dumper"), GOLD, 1);
    CFill(WIN_X + 12, WIN_Y + 22, WIN_W - 24, 1, GOLD);
    u32 size = DUMP_SIZES[g_dumpSizeIdx];
    int x = WIN_X + 16, y = WIN_Y + 30; char l[72];
    sniprintf(l, sizeof l, "%s  0x%08lX", T("Start:"), (unsigned long)g_dumpStart);        CText6(x, y, l, INK); y += 15;
    sniprintf(l, sizeof l, "%s  0x%08lX", T("End:"), (unsigned long)(g_dumpStart + size)); CText6(x, y, l, INK); y += 15;
    sniprintf(l, sizeof l, "%s  %s", T("Size:"), DUMP_SIZE_NM[g_dumpSizeIdx]);             CText6(x, y, l, INK); y += 19;
    CText6(x, y, T("Saves to:"), INK_DIM); y += 13;
    CText6(x, y, PlgPath(DUMP_LEAF "/"), INK_DIM); y += 13;
    CText6(x, y, "  dump_<start>_<size>.bin", INK_DIM); y += 19;
    if (status) CText6(x, y, T(status), sr, sg, sb);
    y += 16;
    if (pct >= 0)
    {
        int bw = WIN_W - 40, bx = x;
        CFillInset(bx, y, bw, 9, 1);
        CFill(bx, y, bw * pct / 100, 9, 120, 200, 120);
        CFill(bx, y, bw, 1, GOLD); CFill(bx, y + 8, bw, 1, GOLD);
        CFill(bx, y, 1, 9, GOLD);  CFill(bx + bw - 1, y, 1, 9, GOLD);
    }
    CText6Btn(WIN_X + 12, WIN_Y + WIN_H - 14, T("{X} size  {Y} from-search  {R} dump  {B} exit"), INK_DIM);
    Present(); Present();
}

static void RamDumpDrawForm(void)
{
    for (int yy = 0; yy < BOT_H; ++yy)
        for (int xx = 0; xx < BOT_W; ++xx)
        {
            u8 *p = CPix(xx, yy);
            if (savedBotValid)
            { u16 v = savedBot[yy * BOT_W + xx];
              p[0] = (u8)(((v>>11)&31)<<3); p[1] = (u8)(((v>>5)&63)<<2); p[2] = (u8)((v&31)<<3); }
            else { p[0] = p[1] = p[2] = 12; }
        }
    CFillBlend(0, 0, BOT_W, BOT_H, BG, 230);
    CFill(6, 4, BOT_W - 12, 1, GOLD); CFill(6, BOT_H - 6, BOT_W - 12, 1, GOLD);
    CText(14, 8, T("RAM Dumper"), GOLD, 1);
    CFill(14, 30, C6Width(T("RAM Dumper")) * 2, 1, GOLD);

    int lx = 14, vx = 120, vw = 186, fy = 66, fh = 26, g = 12; // fy centers the form block in the lower area
    const char *labels[2] = { "Start Addr", "Size" };
    char val[2][40];
    sniprintf(val[0], sizeof val[0], "0x%08lX", (unsigned long)g_dumpStart);
    sniprintf(val[1], sizeof val[1], "%s", DUMP_SIZE_NM[g_dumpSizeIdx]);
    for (int i = 0; i < 2; ++i)
    {
        int y = fy + i * (fh + g);
        CText6(lx, y + 6, T(labels[i]), INK);
        CFillInset(vx, y, vw, fh, 0);
        CFill(vx, y, vw, 1, GOLD); CFill(vx, y + fh - 1, vw, 1, GOLD);
        CFill(vx, y, 1, fh, GOLD); CFill(vx + vw - 1, y, 1, fh, GOLD);
        CText6(vx + 6, y + 6, val[i], INK);
    }
    int by = fy + 2 * (fh + g) + 10;
    KbKey(14,  by, 140, 32, T("From Search"), 4, 0);
    KbKey(166, by, 140, 32, T("Dump"), 5, 0);
    const char *hint = T("Tap a field, or use the buttons");
    CText6((BOT_W - C6Width(hint)) / 2, by + 38, hint, INK_DIM);
    BotBlitComposeBoth();
}

typedef struct { int x, y, w, h, code; } FBox2;
static int RamDumpBuildForm(FBox2 *f)
{
    int n = 0, vx = 120, vw = 186, fy = 66, fh = 26, g = 12; // fy must match ComposeRamDump above
    f[n++] = (FBox2){ vx, fy + 0 * (fh + g), vw, fh, 1 }; // start
    f[n++] = (FBox2){ vx, fy + 1 * (fh + g), vw, fh, 2 }; // size
    int by = fy + 2 * (fh + g) + 10;
    f[n++] = (FBox2){ 14,  by, 140, 32, 3 };              // From Search
    f[n++] = (FBox2){ 166, by, 140, 32, 4 };              // Dump
    return n;
}

static void DoDump(const char **status, u8 *sr, u8 *sg, u8 *sb)
{
    u32 want = DUMP_SIZES[g_dumpSizeIdx];
    u32 span = ReadableSpan(g_dumpStart, want);
    if (!span)      { *status = "Address not readable - try another Start"; *sr=236; *sg=140; *sb=120; return; }
    FsBootInit();
    if (!fsReady)   { *status = "SD card not available"; *sr=236; *sg=140; *sb=120; return; }
    char dir[320];
    { const char *d = PlgPath(DUMP_LEAF); int i = 0; while (d[i] && i < 319) { dir[i] = d[i]; i++; } dir[i] = 0; }
    FSUSER_CreateDirectory(cfgArchive, fsMakePath(PATH_ASCII, dir), 0); // ok if it exists
    char path[360];
    sniprintf(path, sizeof path, "%s/dump_%08lX_%luK.bin", dir,
              (unsigned long)g_dumpStart, (unsigned long)(span / 1024));
    Handle fh;
    if (R_FAILED(FSUSER_OpenFile(&fh, cfgArchive, fsMakePath(PATH_ASCII, path),
                                 FS_OPEN_WRITE | FS_OPEN_CREATE, 0)))
    { *status = "Could not create file"; *sr=236; *sg=140; *sb=120; return; }
    FSFILE_SetSize(fh, span);
    u32 off = 0, chunk = 0x40000; Result r = 0; int lastPct = -1;
    while (off < span)
    {
        u32 n = span - off; if (n > chunk) n = chunk;
        u32 wrote = 0;
        r = FSFILE_Write(fh, &wrote, off, (const void *)(g_dumpStart + off), n, 0);
        if (R_FAILED(r)) break;
        off += n;
        int pct = (int)((u64)off * 100 / span);
        if (pct != lastPct) { lastPct = pct; RamDumpDrawTop("Dumping...", 236, 200, 120, pct); }
    }
    FSFILE_Close(fh);
    if (R_FAILED(r)) { *status = "Write error"; *sr=236; *sg=140; *sb=120; return; }
    static char msg[80];
    if (span < want) siprintf(msg, "Saved %luKB (clamped to readable memory)", (unsigned long)(span / 1024));
    else             siprintf(msg, "Saved %luKB to dumps/dump_%08lX_%luK.bin",
                              (unsigned long)(span / 1024), (unsigned long)g_dumpStart, (unsigned long)(span / 1024));
    *status = msg; *sr=140; *sg=236; *sb=120;
    QueueToastRaw("RAM dumped", "");
}

static void ToolRamDump(void)
{
    KbInit();
    if (g_dumpStart == 0x08000000 && g_searchSelAddr) g_dumpStart = g_searchSelAddr; // handy default
    int redrawTop = 1, redrawBot = 1;
    u32 prev = HID_PAD; int touchPrev = HidTouch(0, 0);
    const char *status = NULL; u8 sr = 140, sg = 236, sb = 120;

    while (1)
    {
        if (redrawTop) { RamDumpDrawTop(status, sr, sg, sb, -1); redrawTop = 0; }
        if (redrawBot) { RamDumpDrawForm(); redrawBot = 0; }

        svcSleepThread(16 * 1000 * 1000);
        u32 pad = HID_PAD, down = ARepeat(pad, &prev, &g_arHold);

        if (down & BUTTON_B) break;
        if (down & BUTTON_SELECT) { g_quitToGame = 1; break; }
        if (down & BUTTON_X) { g_dumpSizeIdx = (g_dumpSizeIdx + 1) % NUM_DUMP_SIZES; redrawTop = 1; redrawBot = 1; }
        if (down & BUTTON_Y)
        {
            if (g_searchSelAddr) { g_dumpStart = g_searchSelAddr; status = "Loaded address from Cheat Search"; sr=236; sg=200; sb=120; }
            else                 { status = "No Cheat Search result yet"; sr=236; sg=180; sb=120; }
            redrawTop = 1; redrawBot = 1;
        }
        if (down & BUTTON_R1) { DoDump(&status, &sr, &sg, &sb); redrawTop = 1; redrawBot = 1; prev = HID_PAD; continue; }

        int px, py, now = HidTouch(&px, &py);
        int tap = now && !touchPrev; touchPrev = now;
        if (!hidReady || !tap) continue;
        FBox2 f[6]; int nf = RamDumpBuildForm(f);
        for (int i = 0; i < nf; ++i)
        {
            if (!KbHit(px, py, f[i].x, f[i].y, f[i].w, f[i].h)) continue;
            switch (f[i].code)
            {
                case 1: { int c; u32 v = EnterNum("Start address", g_dumpStart, &c); if (!c) g_dumpStart = v; } break;
                case 2: g_dumpSizeIdx = (g_dumpSizeIdx + 1) % NUM_DUMP_SIZES; break;
                case 3:
                    if (g_searchSelAddr) { g_dumpStart = g_searchSelAddr; status = "Loaded address from Cheat Search"; sr=236; sg=200; sb=120; }
                    else                 { status = "No Cheat Search result yet"; sr=236; sg=180; sb=120; }
                    break;
                case 4: DoDump(&status, &sr, &sg, &sb); break;
            }
            redrawTop = 1; redrawBot = 1;
            touchPrev = HidTouch(0, 0);
            break;
        }
    }
}

// ---- Hex Editor ----
#define HEX_COLS 8
#define HEX_ROWS 11
#define HEX_LO   0x00100000u
#define HEX_HI   0x40000000u
static u32 g_hexCursor = 0x08000000;
static u32 g_hexView   = 0x08000000;

// cached readable window so a page of bytes costs ~1 svcQueryMemory, not 88.
static u32 g_mrLo = 1, g_mrHi = 0; static int g_mrOk = 0;
static int MemReadable(u32 a)
{
    if (a >= g_mrLo && a < g_mrHi) return g_mrOk;
    MemInfo info; PageInfo pg;
    if (R_FAILED(svcQueryMemory(&info, &pg, a))) { g_mrLo = a & ~0xFFFu; g_mrHi = g_mrLo + 0x1000; g_mrOk = 0; return 0; }
    g_mrLo = info.base_addr; g_mrHi = info.base_addr + info.size;
    g_mrOk = (info.perm & MEMPERM_READ) && info.state != MEMSTATE_FREE;
    return g_mrOk;
}
static int MemWritable(u32 a)
{
    MemInfo info; PageInfo pg;
    if (R_FAILED(svcQueryMemory(&info, &pg, a))) return 0;
    return (info.perm & MEMPERM_WRITE) && info.state != MEMSTATE_FREE;
}

static void HexClampCursor(void)
{
    if (g_hexCursor < HEX_LO) g_hexCursor = HEX_LO;
    if (g_hexCursor > HEX_HI - 1) g_hexCursor = HEX_HI - 1;
    u32 span = HEX_ROWS * HEX_COLS;
    u32 curRow = g_hexCursor - (g_hexCursor % HEX_COLS);
    if (g_hexCursor < g_hexView) g_hexView = curRow;
    else if (g_hexCursor >= g_hexView + span) g_hexView = curRow - (span - HEX_COLS);
    if (g_hexView < HEX_LO) g_hexView = HEX_LO;
}

static void HexDrawTop(void)
{
    ComposeBackdrop();
    CText(WIN_X + 12, WIN_Y + 6, T("Hex Editor"), GOLD, 1);
    char h[32];
    int rd = MemReadable(g_hexCursor);
    siprintf(h, "%08lX = %02X", (unsigned long)g_hexCursor, rd ? R8(g_hexCursor) : 0);
    CText6(WIN_X + WIN_W - 12 - C6Width(h), WIN_Y + 9, h, rd ? 236 : 200, rd ? 236 : 150, rd ? 210 : 120);
    CFill(WIN_X + 12, WIN_Y + 22, WIN_W - 24, 1, GOLD);

    int ax = WIN_X + 12, hx0 = ax + 62, asc0 = hx0 + HEX_COLS * 20 + 6;
    for (int r = 0; r < HEX_ROWS; ++r)
    {
        u32 rowAddr = g_hexView + (u32)r * HEX_COLS;
        int y = WIN_Y + 28 + r * 13;
        char al[10]; siprintf(al, "%08lX", (unsigned long)rowAddr);
        CText6(ax, y, al, INK_DIM);
        for (int c = 0; c < HEX_COLS; ++c)
        {
            u32 a = rowAddr + c;
            int rdb = MemReadable(a);
            int bx = hx0 + c * 20;
            if (a == g_hexCursor)
            {
                CFillBlend(bx - 2, y - 1, 17, 12, 0, 0, 0, 150);
                CFill(bx - 2, y - 1, 17, 1, GOLD); CFill(bx - 2, y + 10, 17, 1, GOLD);
                CFill(bx - 2, y - 1, 1, 12, GOLD); CFill(bx + 14, y - 1, 1, 12, GOLD);
            }
            char bb[4]; if (rdb) siprintf(bb, "%02X", R8(a)); else siprintf(bb, "--");
            CText6(bx, y, bb, rdb ? 236 : 96, rdb ? 236 : 80, rdb ? 210 : 60);
            u8 v = rdb ? R8(a) : 0;
            char ch[2]; ch[0] = (v >= 32 && v < 127) ? (char)v : '.'; ch[1] = 0;
            CText6(asc0 + c * 7, y, ch, a == g_hexCursor ? 236 : 150, a == g_hexCursor ? 200 : 140, a == g_hexCursor ? 120 : 112);
        }
    }
    CText6Btn(WIN_X + 12, WIN_Y + WIN_H - 14, T("{DP} move  {A} edit  {X} goto  {B} exit"), INK_DIM);
    Present(); Present();
}

static void HexDrawForm(void)
{
    for (int yy = 0; yy < BOT_H; ++yy)
        for (int xx = 0; xx < BOT_W; ++xx)
        {
            u8 *p = CPix(xx, yy);
            if (savedBotValid)
            { u16 v = savedBot[yy * BOT_W + xx];
              p[0] = (u8)(((v>>11)&31)<<3); p[1] = (u8)(((v>>5)&63)<<2); p[2] = (u8)((v&31)<<3); }
            else { p[0] = p[1] = p[2] = 12; }
        }
    CFillBlend(0, 0, BOT_W, BOT_H, BG, 230);
    CFill(6, 4, BOT_W - 12, 1, GOLD); CFill(6, BOT_H - 6, BOT_W - 12, 1, GOLD);
    CText(14, 8, T("Hex Editor"), GOLD, 1);
    CFill(14, 30, C6Width(T("Hex Editor")) * 2, 1, GOLD);

    int lx = 14, vx = 120, vw = 186, fy = 46, fh = 26, g = 12;
    const char *labels[2] = { "Address", "Byte @ cursor" };
    char val[2][40];
    sniprintf(val[0], sizeof val[0], "0x%08lX", (unsigned long)g_hexCursor);
    if (MemReadable(g_hexCursor)) sniprintf(val[1], sizeof val[1], "0x%02X  (%u)", R8(g_hexCursor), R8(g_hexCursor));
    else                          sniprintf(val[1], sizeof val[1], "%s", T("-- (unreadable)"));
    for (int i = 0; i < 2; ++i)
    {
        int y = fy + i * (fh + g);
        CText6(lx, y + 6, T(labels[i]), INK);
        CFillInset(vx, y, vw, fh, 0);
        CFill(vx, y, vw, 1, GOLD); CFill(vx, y + fh - 1, vw, 1, GOLD);
        CFill(vx, y, 1, fh, GOLD); CFill(vx + vw - 1, y, 1, fh, GOLD);
        CText6(vx + 6, y + 6, val[i], INK);
    }
    int by = fy + 2 * (fh + g) + 10;
    KbKey(14,  by, 140, 32, T("From Search"), 4, 0);
    KbKey(166, by, 140, 32, T("Edit Byte"), 5, 0);
    const char *hint = T("On top: {DP} move  {L}/{R} page  {Y} from-search");
    CText6Btn((BOT_W - C6BtnWidth(hint)) / 2, by + 38, hint, INK_DIM);
    BotBlitComposeBoth();
}

typedef struct { int x, y, w, h, code; } FBox3;
static int HexBuildForm(FBox3 *f)
{
    int n = 0, vx = 120, vw = 186, fy = 46, fh = 26, g = 12;
    f[n++] = (FBox3){ vx, fy + 0 * (fh + g), vw, fh, 1 }; // address (goto)
    f[n++] = (FBox3){ vx, fy + 1 * (fh + g), vw, fh, 2 }; // byte (edit)
    int by = fy + 2 * (fh + g) + 10;
    f[n++] = (FBox3){ 14,  by, 140, 32, 3 };              // From Search
    f[n++] = (FBox3){ 166, by, 140, 32, 4 };              // Edit Byte
    return n;
}

static void HexEditByte(void)
{
    if (!MemWritable(g_hexCursor)) { QueueToastRaw("Read-only here", ""); return; }
    int c; u32 v = EnterNum("Edit byte", MemReadable(g_hexCursor) ? R8(g_hexCursor) : 0, &c);
    if (!c) { W8(g_hexCursor, (u8)v); QueueToastRaw("Byte written", ""); }
}
static void HexGoto(void)
{
    int c; u32 a = EnterNum("Go to address", g_hexCursor, &c);
    if (!c) { g_hexCursor = a; HexClampCursor(); }
}

static void ToolHexEdit(void)
{
    KbInit();
    g_mrLo = 1; g_mrHi = 0; // reset readable cache
    if (g_searchSelAddr) g_hexCursor = g_searchSelAddr;   // start where the user was looking
    HexClampCursor(); g_hexView = g_hexCursor - (g_hexCursor % HEX_COLS);
    HexClampCursor();

    int redrawTop = 1, redrawBot = 1;
    u32 prev = HID_PAD; int touchPrev = HidTouch(0, 0);

    while (1)
    {
        if (redrawTop) { HexDrawTop(); redrawTop = 0; }
        if (redrawBot) { HexDrawForm(); redrawBot = 0; }

        svcSleepThread(16 * 1000 * 1000);
        u32 pad = HID_PAD, down = ARepeat(pad, &prev, &g_arHold);

        if (down & BUTTON_B) break;
        if (down & BUTTON_SELECT) { g_quitToGame = 1; break; }

        int moved = 0;
        if (down & BUTTON_LEFT)  { g_hexCursor -= 1; moved = 1; }
        if (down & BUTTON_RIGHT) { g_hexCursor += 1; moved = 1; }
        if (down & BUTTON_UP)    { g_hexCursor -= HEX_COLS; moved = 1; }
        if (down & BUTTON_DOWN)  { g_hexCursor += HEX_COLS; moved = 1; }
        if (down & BUTTON_L1)    { g_hexCursor -= HEX_ROWS * HEX_COLS; moved = 1; }
        if (down & BUTTON_R1)    { g_hexCursor += HEX_ROWS * HEX_COLS; moved = 1; }
        if (moved) { HexClampCursor(); redrawTop = 1; redrawBot = 1; }

        if (down & BUTTON_A) { HexEditByte(); redrawTop = 1; redrawBot = 1; prev = HID_PAD; continue; }
        if (down & BUTTON_X) { HexGoto();     redrawTop = 1; redrawBot = 1; prev = HID_PAD; continue; }
        if (down & BUTTON_Y)
        {
            if (g_searchSelAddr) { g_hexCursor = g_searchSelAddr; HexClampCursor(); }
            else QueueToastRaw("No Cheat Search result yet", "");
            redrawTop = 1; redrawBot = 1;
        }

        int px, py, now = HidTouch(&px, &py);
        int tap = now && !touchPrev; touchPrev = now;
        if (!hidReady || !tap) continue;
        FBox3 f[6]; int nf = HexBuildForm(f);
        for (int i = 0; i < nf; ++i)
        {
            if (!KbHit(px, py, f[i].x, f[i].y, f[i].w, f[i].h)) continue;
            switch (f[i].code)
            {
                case 1: HexGoto(); break;
                case 2: HexEditByte(); break;
                case 3:
                    if (g_searchSelAddr) { g_hexCursor = g_searchSelAddr; HexClampCursor(); }
                    else QueueToastRaw("No Cheat Search result yet", "");
                    break;
                case 4: HexEditByte(); break;
            }
            redrawTop = 1; redrawBot = 1;
            touchPrev = HidTouch(0, 0);
            break;
        }
    }
}

// ======================= Game Guide / Plugin Guide =======================
// Reader: word-wrap a body into visual lines, then scroll through them.
#define GR_MAXLINES 2000
static u16 g_glOff[GR_MAXLINES];
static u16 g_glLen[GR_MAXLINES];
static int g_glN;
static void GuideWrap(const char *s, int cols)
{
    g_glN = 0;
    int len = 0; while (s[len]) len++;
    int i = 0;
    while (i < len && g_glN < GR_MAXLINES)
    {
        int start = i, lastSpace = -1, count = 0;
        while (i < len && count < cols && s[i] != '\n')
        {
            if (s[i] == ' ') lastSpace = i;
            i++; count++;
        }
        int end;
        if (i < len && s[i] == '\n')                  { end = i; i++; }               // hard break
        else if (i >= len)                            { end = i; }                     // end of text
        else if (count >= cols && lastSpace > start)  { end = lastSpace; i = lastSpace + 1; } // wrap at space
        else                                          { end = i; }                     // long word / hard cut
        g_glOff[g_glN] = (u16)start; g_glLen[g_glN] = (u16)(end - start); g_glN++;
    }
}

// static branded bottom panel while a guide is open
// Bottom-screen header for BOTH readers. The title used to be hardcoded to "Game Guide",
// which meant the Plugin Guide announced itself as the Game Guide - take it as a parameter.
static void GuideBottom(const char *title, const char *subtitle)
{
    for (int yy = 0; yy < BOT_H; ++yy)
        for (int xx = 0; xx < BOT_W; ++xx)
        {
            u8 *p = CPix(xx, yy);
            if (savedBotValid)
            { u16 v = savedBot[yy * BOT_W + xx];
              p[0] = (u8)(((v>>11)&31)<<3); p[1] = (u8)(((v>>5)&63)<<2); p[2] = (u8)((v&31)<<3); }
            else { p[0] = p[1] = p[2] = 12; }
        }
    CFillBlend(0, 0, BOT_W, BOT_H, BG, 230);
    CFill(6, 4, BOT_W - 12, 1, GOLD); CFill(6, BOT_H - 6, BOT_W - 12, 1, GOLD);
    CText(14, 10, title, GOLD, 1);
    CFill(14, 32, C6Width(title) * 2, 1, GOLD);
    CText6(14, 44, subtitle, INK);   // guide content: stays English
    CText6(14, 70, T("Read on the top screen."), INK_DIM);
    CText6Btn(14, 86, T("{DP} / {L}/{R} : scroll or move"), INK_DIM);
    CText6Btn(14, 100, T("{A} open     {B} back"), INK_DIM);
    CText6(14, 114, T("SELECT : back to the game"), INK_DIM);
    BotBlitComposeBoth();
}

// Solid themed window with an accent border, matching the rest of the UI,
// so dense guide text is easy to read.
static void GuideBackdrop(void)
{
    RestoreTopBackdrop();
    CFill(WIN_X, WIN_Y, WIN_W, WIN_H, BG); // solid theme background for easy reading
    CFill(WIN_X, WIN_Y, WIN_W, 2, GOLD);
    CFill(WIN_X, WIN_Y + WIN_H - 2, WIN_W, 2, GOLD);
    CFill(WIN_X, WIN_Y, 2, WIN_H, GOLD);
    CFill(WIN_X + WIN_W - 2, WIN_Y, 2, WIN_H, GOLD);
}

// Scrollable reader. Returns 1 on B (back); sets g_quitToGame and returns 0 on SELECT.
static int GuideReader(const char *title, const char *body, int *scrollIO)
{
    int cols = (WIN_W - 30) / 7;   // chars/line at 7px advance (~41)
    GuideWrap(body, cols);
    int rows = 12, redraw = 1;
    int scroll = scrollIO ? *scrollIO : 0;
    if (scroll > g_glN) scroll = 0;
    u32 prev = HID_PAD;
    while (1)
    {
        int maxScroll = (g_glN > rows) ? g_glN - rows : 0;
        if (redraw)
        {
            GuideBackdrop();
            CText(WIN_X + 12, WIN_Y + 6, title, GOLD, 1);
            char pi[16];
            siprintf(pi, "%d%%", maxScroll ? scroll * 100 / maxScroll : 100);
            CText6(WIN_X + WIN_W - 12 - C6Width(pi), WIN_Y + 9, pi, INK_DIM);
            CFill(WIN_X + 12, WIN_Y + 22, WIN_W - 24, 1, GOLD);
            for (int r = 0; r < rows; ++r)
            {
                int li = scroll + r;
                if (li >= g_glN) break;
                char buf[64];
                int len = g_glLen[li]; if (len > 63) len = 63;
                memcpy(buf, body + g_glOff[li], (size_t)len); buf[len] = 0;
                CText6(WIN_X + 14, WIN_Y + 28 + r * 13, buf, INK);
            }
            if (g_glN > rows)
            {
                int trackH = rows * 13;
                int barH = trackH * rows / g_glN; if (barH < 8) barH = 8;
                int barY = WIN_Y + 28 + (trackH - barH) * scroll / maxScroll;
                CFillInset(WIN_X + WIN_W - 15, WIN_Y + 28, 3, trackH, 1);
                CFill(WIN_X + WIN_W - 15, barY, 3, barH, GOLD);
            }
            CText6Btn(WIN_X + 12, WIN_Y + WIN_H - 14, T("{DP} / {L}/{R} scroll   {B} back"), INK_DIM);
            Present(); Present();
            redraw = 0;
        }
        svcSleepThread(16 * 1000 * 1000);
        u32 pad = HID_PAD, down = ARepeat(pad, &prev, &g_arHold);
        if ((down & BUTTON_DOWN) && scroll < maxScroll) { scroll++; redraw = 1; }
        if ((down & BUTTON_UP)   && scroll > 0)         { scroll--; redraw = 1; }
        if (down & BUTTON_R1) { scroll += rows; if (scroll > maxScroll) scroll = maxScroll; redraw = 1; }
        if (down & BUTTON_L1) { scroll -= rows; if (scroll < 0) scroll = 0; redraw = 1; }
        if (down & BUTTON_B) { if (scrollIO) *scrollIO = scroll; return 1; }
        if (down & BUTTON_SELECT) { if (scrollIO) *scrollIO = scroll; g_quitToGame = 1; return 0; }
    }
}

// Titled list with a cursor. Same look as the main menu (system font, folder
// icons, ROW_H rows). Returns chosen index, or -1 on B, -2 on SELECT (quit).
static int GuideList(const char *title, const char **labels, int count, int initSel, int *outSel)
{
    int sel = (initSel >= 0 && initSel < count) ? initSel : 0, scroll = 0, redraw = 1;
    u32 prev = HID_PAD;
    while (1)
    {
        if (sel < scroll) scroll = sel;
        if (sel >= scroll + MAX_ROWS) scroll = sel - MAX_ROWS + 1;
        if (outSel) *outSel = sel;
        if (redraw)
        {
            ComposeBackdrop();
            int tw = CTextWidth(title);
            CText(WIN_X + 12, WIN_Y + 7, title, INK, 1);
            CFill(WIN_X + 12, WIN_Y + 24, tw + 6, 1, GOLD);
            for (int r = 0; r < MAX_ROWS; ++r)
            {
                int i = scroll + r; if (i >= count) break;
                int y = ROW_Y0 + r * ROW_H;
                if (i == sel)
                {
                    CFillBlend(ROW_X - 4, y - 1, ROW_W + 8, ROW_H, 0, 0, 0, 110);
                    CFill(ROW_X - 4, y - 1, 2, ROW_H, GOLD);
                }
                FolderIconSmall(ROW_X, y + 1);
                CText(ROW_X + 20, y - 1, labels[i], INK, 0);
            }
            if (scroll > 0)
                for (int a = 0; a < 4; ++a) CFill(WIN_X + WIN_W - 14 - a, ROW_Y0 + 3 + a, 1 + 2 * a, 1, GOLD);
            if (scroll + MAX_ROWS < count)
                for (int a = 0; a < 4; ++a) CFill(WIN_X + WIN_W - 14 - a, ROW_Y0 + MAX_ROWS * ROW_H - 4 - a, 1 + 2 * a, 1, GOLD);
            CText6Btn(WIN_X + 12, WIN_Y + WIN_H - 16, T("{A} open   {B} back   SELECT: game"), INK_DIM);
            Present(); Present();
            redraw = 0;
        }
        svcSleepThread(16 * 1000 * 1000);
        u32 pad = HID_PAD, down = ARepeat(pad, &prev, &g_arHold);
        if (down & BUTTON_DOWN) { sel = (sel + 1 < count) ? sel + 1 : 0; redraw = 1; }
        if (down & BUTTON_UP)   { sel = (sel > 0) ? sel - 1 : count - 1; redraw = 1; }
        if (down & BUTTON_A)      return sel;
        if (down & BUTTON_B)      return -1;
        if (down & BUTTON_SELECT) return -2;
    }
}

#if !TOOLS_ONLY
// Credits page for the Game Guide. If you ship someone else's walkthrough text,
// THIS is where you credit them - name the author, where it came from, and under
// what permission. Replace the placeholder below before you publish.
static const char *GUIDE_CREDITS =
    "Replace this page with credits for your guide content.\n"
    "\n"
    "If the walkthrough text is not yours, say so here:\n"
    "  - who wrote it\n"
    "  - where it came from\n"
    "  - that you have permission to redistribute it\n"
    "\n"
    "Do the same for anything else you build on: address maps, save-data\n"
    "research, art, translations. It costs one screen and it is the difference\n"
    "between a fan project and a rip.\n"
    "\n"
    "Game names and game content belong to their publisher.";

// Navigation state persists so that SELECT-to-game then SELECT-back returns you
// to the exact page and scroll position you were reading.
// mode: 0 = category list, 1 = page list, 2 = reader, 3 = credits reader.
static int g_ggMode = 0, g_ggCatCur = 0, g_ggCat = 0, g_ggPage = 0, g_ggScroll = 0, g_ggCredScroll = 0;
static void ToolGameGuide(void)
{
    GuideBottom(T("Game Guide"), T("Your game's content"));
    while (1)
    {
        int ncats; const GuideCat *cats = GG_Cats(&ncats);
        if (g_ggCat >= ncats) g_ggCat = 0;              // language switch may shrink the set
        if (g_ggMode == 2) // reading a category page
        {
            if (g_ggPage >= cats[g_ggCat].nPages) g_ggPage = 0;
            const GuidePage *pg = &cats[g_ggCat].pages[g_ggPage];
            int r = GuideReader(pg->title, pg->body, &g_ggScroll);
            if (r == 0) return;          // SELECT: stay at mode 2 -> resume here next time
            g_ggMode = 1;                // B -> page list
        }
        else if (g_ggMode == 3) // reading Credits
        {
            int r = GuideReader("Credits", GUIDE_CREDITS, &g_ggCredScroll);
            if (r == 0) return;
            g_ggMode = 0;
        }
        else if (g_ggMode == 0) // category list
        {
            const char *labels[SDG_MAXCATS + 1];
            for (int i = 0; i < ncats; ++i) labels[i] = cats[i].title;
            labels[ncats] = "Credits";
            int r = GuideList(T("Game Guide"), labels, ncats + 1, g_ggCatCur, &g_ggCatCur);
            if (r == -2) { g_quitToGame = 1; return; } // stay at mode 0 -> resume the list
            if (r == -1) return;
            if (r == ncats) g_ggMode = 3;              // Credits
            else { g_ggCat = r; g_ggMode = 1; }
        }
        else // page list
        {
            const GuideCat *c = &cats[g_ggCat];
            const char *labels[20];
            int n = c->nPages; if (n > 20) n = 20;
            for (int i = 0; i < n; ++i) labels[i] = c->pages[i].title;
            int r = GuideList(c->title, labels, n, g_ggPage, &g_ggPage);
            if (r == -2) { g_quitToGame = 1; return; }
            if (r == -1) { g_ggMode = 0; continue; }
            g_ggPage = r; g_ggScroll = 0; g_ggMode = 2; // open the page from the top
        }
    }
}

#endif // !TOOLS_ONLY

// ---- Plugin Guide (original content, explains this plugin) ----
#if TOOLS_ONLY
// Pages for the universal (default.3gx) build. Nothing here may mention cheats, the tracker or
// the game guide - none of them exist in this binary, and a guide that describes menus you do
// not have is worse than no guide.
static const GuidePage PLUGIN_PAGES[] = {
    { "What this is",
      "CTRComposer Tools is a single plugin that loads into every title on the system, from\n"
      "sd:/luma/plugins/default.3gx.\n"
      "\n"
      "Press SELECT during any game to open this menu. The game pauses while it is open.\n"
      "Press SELECT again, from anywhere, to jump straight back.\n"
      "\n"
      "It carries no cheats, and that is not an oversight: a cheat is a memory address, and an\n"
      "address belongs to one game and one region. What IS universal is the tooling - searching\n"
      "memory, reading it, editing it - so that is what this build carries.\n"
      "\n"
      "Navigate with the D-Pad. A opens a tool, B goes back, X shows info about the selected\n"
      "row, Y stars it as a favourite." },
    { "Cheat Search",
      "Find the memory address of any value, then change it. This works on any game, because\n"
      "it scans memory rather than knowing anything about the title.\n"
      "\n"
      "Known Value: type a number you can see (health, coins, a timer), Search, then narrow the\n"
      "results as the value changes (Greater / Less / Changed...).\n"
      "\n"
      "Unknown Search: don't know the number? Take a snapshot, change the value in game, then\n"
      "scan Increased / Decreased / Changed to close in on it.\n"
      "\n"
      "The real loop: Search, press SELECT to return to the game, change the value, SELECT to\n"
      "reopen (results are kept), scan again. Repeat until a few results remain. Press A on a\n"
      "result to poke a new value. L undoes a scan." },
    { "RAM Dumper",
      "Save a block of memory to a .bin file on the SD card.\n"
      "\n"
      "Set a Start address (or press Y / From Search to pull the address you found in Cheat\n"
      "Search) and a Size, then Dump. Files are written to the plugin's own folder, under\n"
      "dumps/.\n"
      "\n"
      "The tool only writes memory that is actually readable, so it never crashes on an\n"
      "unmapped address. Good for studying the bytes around a value you found." },
    { "Hex Editor",
      "Browse memory as a live hex grid and edit any byte on the spot.\n"
      "\n"
      "D-Pad moves the cursor (left/right one byte, up/down one row). L/R page up and down.\n"
      "X jumps to an address; Y jumps to your Cheat Search result. Press A to edit the byte\n"
      "under the cursor.\n"
      "\n"
      "Read-only regions are protected: editing there is refused instead of crashing.\n"
      "Unreadable bytes show as --." },
    { "Quick menu & tips",
      "Star a tool with Y and it appears in the quick menu: hold L+SELECT (or R+SELECT) in\n"
      "game to launch it without opening the full menu. The combo is configurable in Settings.\n"
      "\n"
      "- SELECT is always 'back to the game', from any screen.\n"
      "- Reopening the menu returns you to where you were, even inside a tool.\n"
      "- Search results survive closing the menu, and even closing this menu entirely.\n"
      "- Settings, favourites and your theme are saved to the SD card.\n"
      "\n"
      "This build loads into everything, including the Home Menu and homebrew. If something\n"
      "misbehaves, delete default.3gx and check whether it still happens." },
};
#else
static const GuidePage PLUGIN_PAGES[] = {
    { "Overview",
      "This plugin draws its own overlay on top of the running game.\n"
      "\n"
      "Press SELECT during the game to open the menu. The game pauses while the\n"
      "menu is open. Press SELECT again (from anywhere) to jump straight back to\n"
      "the game.\n"
      "\n"
      "Navigate with the D-Pad. A opens a folder or toggles a cheat. B goes back\n"
      "one level. X shows info about the selected item. Y stars a favorite." },
    { "Quick Menu & Favorites",
      "Star your most-used cheats with Y in the menu. Then hold L+SELECT (or\n"
      "R+SELECT) to open the Quick Menu: a compact list of just your favorites,\n"
      "without opening the full menu.\n"
      "\n"
      "The hotkey can be changed in Settings. Favorites, the toast toggle and the\n"
      "hotkey are saved to the SD card and survive a reboot." },
    { "Cheat Search",
      "Find the memory address of any value, then change it.\n"
      "\n"
      "Known Value: type a number you can see (e.g. your rupees), Search, then\n"
      "narrow the results as the value changes (Greater / Less / Changed...).\n"
      "\n"
      "Unknown Search: don't know the number? Take a snapshot, change the value\n"
      "in the game, then scan Increased / Decreased / Changed to close in on it.\n"
      "\n"
      "The real loop: Search, press SELECT to return to the game, change the\n"
      "value, SELECT to reopen (results are kept), scan again. Repeat until a few\n"
      "results remain. Press A on a result to poke a new value. L undoes a scan." },
    { "RAM Dumper",
      "Save a block of the game's memory to a .bin file on the SD card.\n"
      "\n"
      "Set a Start address (or press Y / From Search to pull the address you\n"
      "found in Cheat Search) and a Size, then Dump. Files are written to\n"
      "the plugin's own folder on the SD card, under dumps/.\n"
      "\n"
      "The tool only writes memory that is actually readable, so it never\n"
      "crashes on an unmapped address. Great for studying the bytes around a\n"
      "value you found." },
    { "Hex Editor",
      "Browse memory as a live hex grid and edit any byte on the spot.\n"
      "\n"
      "D-Pad moves the cursor (left/right one byte, up/down one row). L/R page\n"
      "up and down. X jumps to an address; Y jumps to your Cheat Search result.\n"
      "Press A to edit the byte under the cursor.\n"
      "\n"
      "Read-only regions are protected: editing there is refused instead of\n"
      "crashing. Unreadable bytes show as --." },
    { "Tips",
      "- SELECT is always 'back to the game', from any screen.\n"
      "- Reopening the menu returns you to where you were, even inside a tool.\n"
      "- Code-patch cheats are never auto-enabled on boot, by design.\n"
      "- Toast notifications can be turned off in Settings." },
};
#endif
#define PLUGIN_NPAGES ((int)(sizeof(PLUGIN_PAGES) / sizeof(PLUGIN_PAGES[0])))

// Return the active guide model: SD translation if loaded, else embedded English.
#if !TOOLS_ONLY
static const GuideCat *GG_Cats(int *n)
{
    if (g_ggNCats) { *n = g_ggNCats; return g_ggCatsBuf; }
    *n = GUIDE_NCATS; return GUIDE_CATS;
}
#endif
static const GuidePage *PG_Pages(int *n)
{
    if (g_pgNPages) { *n = g_pgNPages; return g_pgPagesBuf; }
    *n = PLUGIN_NPAGES; return PLUGIN_PAGES;
}

static int g_pgMode = 0, g_pgCur = 0, g_pgPage = 0, g_pgScroll = 0; // resume state
static void ToolPluginGuide(void)
{
    GuideBottom(T("Plugin Guide"), T("How to use this plugin"));
    while (1)
    {
        int npg; const GuidePage *pages = PG_Pages(&npg);
        if (g_pgPage >= npg) g_pgPage = 0;
        if (g_pgMode == 1) // reading a page
        {
            int r = GuideReader(pages[g_pgPage].title, pages[g_pgPage].body, &g_pgScroll);
            if (r == 0) return;   // SELECT: resume here
            g_pgMode = 0;
        }
        else
        {
            const char *labels[32];
            int n = npg; if (n > 32) n = 32;
            for (int i = 0; i < n; ++i) labels[i] = pages[i].title;
            int r = GuideList(T("Plugin Guide"), labels, n, g_pgCur, &g_pgCur);
            if (r == -2) { g_quitToGame = 1; return; }
            if (r == -1) return;
            g_pgPage = r; g_pgScroll = 0; g_pgMode = 1;
        }
    }
}

#if !TOOLS_ONLY
// ===================== Completion tracker (per-item progress) =====================
// A general "collectibles / progress" tool, and one of the more useful things the engine
// gives you for free. Each item has one of four states:
//     0 untouched   1 auto-detected   2 you checked it   3 you cleared it
//
// AUTO-FILL = SYNC TO MEMORY, not a running tally. When it runs it re-reads every
// detectable item and both SETS marks it finds and CLEARS stale auto-marks it no longer
// finds, while never touching a mark you made by hand. That is what keeps the tool honest
// after a save reload: it always reflects the CURRENT game state.
//
// Detection kinds - all read-only, and all keyed off addresses you supply:
//   CK_MANUAL  - no memory signal; only you can tick it
//   CK_BIT     - (R8(addr) & mask) != 0            a flag bit inside a byte
//   CK_BYTEEQ  - R8(addr) == mask                  an exact byte value at a FIXED position
//   CK_NONZERO - R8(addr) != 0                     "the slot is filled"
//   CK_SCANEQ  - any R8(addr..addr+scanLen-1) == mask   value appears SOMEWHERE in a range
// Add your own kind by extending this enum and ChecklistAutoFill() together.
//
// >>> THE TABLE BELOW IS EMPTY OF GAME DATA. <<<
// It ships with one "Example" category whose rows are placeholders pointing at address 0,
// purely so the tool is explorable before you have any addresses. Replace CHK_CATS with
// your game's collectibles - it is pure data, and the UI below neither knows nor cares how
// many categories or items there are. If your game has nothing to track, delete the Tracker
// row from rootItems[] and this whole section.
//
// Progress is persisted KEYED BY THE `key` STRING, not by position, so you can add, remove
// and reorder items freely without invalidating anyone's saved progress. Never reuse a key
// for a different item.
enum { CK_MANUAL = 0, CK_BIT = 1, CK_BYTEEQ = 2, CK_NONZERO = 3, CK_SCANEQ = 4 };
// CKI_SPRITE: iconArg is an MSPR_* key from sprites.h - a real rip, not hand-drawn.
// CKI_SWATCH: iconArg is a packed RGB565 color (((r>>3)<<11)|((g>>2)<<5)|(b>>3)) - a plain
// tinted square for items with no matching art on the sheet, in a color close to the name.
enum { CKI_HEART, CKI_SKULL, CKI_NOTE, CKI_KEYITEM, CKI_NONE, CKI_SPRITE, CKI_SWATCH };
#define CHKRGB(r,g,b) (u16)((((r)>>3)<<11)|(((g)>>2)<<5)|((b)>>3))

typedef struct {
    const char *key;    // stable save-key, never shown (so item text can be edited freely later)
    const char *task, *hint, *loc; // loc = "" -> no location to reveal
    u8 iconKind; u16 iconArg; u8 kind;
    u32 addr; u8 mask;
    u8 scanLen;     // CK_SCANEQ only: how many bytes from addr to scan (0/unused for every other kind)
    u16 iconArgBig; // CKI_SPRITE only, optional: a DIFFERENT sprite key for the detail card (cell
                     // > 3) than the list rows use - 0 means "same sprite both places" (the norm;
                     // Owl Statues are the only category that wants a distinct bigger render here)
} ChkItem;
typedef struct { const char *name; const ChkItem *items; int count; } ChkCat;

// Masks (24): CONFIRMED address range 0x77636C-0x776383, 24 bytes total. IMPORTANT: these are
// filled in ACQUISITION ORDER, not one fixed slot per mask - the "Have all Masks" cheat and the
// 100% save cross-check both read 0x32-0x49 ascending only because that save happened to collect
// them in id order. A real save can have any mask's id in any of the 24 slots. So detection scans
// the whole 24-byte range for the target id rather than checking a single fixed offset (this bug
// is exactly why the first version of this Tracker read 0/24 against a real 100% save - fixed by
// switching to CK_SCANEQ). Item ids/names are from the zeldaret/mm N64 decompilation
// (include/z64item.h's ItemId enum), which also matches our own independently-confirmed B-Button
// value for Fierce Deity's Mask (0x35) - so this table is trusted over the earlier community AR
// list. Hints are kept to well-known facts only; anything we weren't confident enough to state
// precisely is left generic rather than risk a wrong walkthrough step.
#define MASKS_BASE 0x77636C
#define MASKS_LEN  24
static const ChkItem CK_MASKS[] = {
    // key                task                     hint                                                                          loc  icon         arg kind       addr        mask  scanLen
    { "mask_deku",        "Deku Mask",              "One of the four transformation masks.",                                       "", CKI_SPRITE, MSPR_MASK_DEKU, CK_SCANEQ, MASKS_BASE, 0x32, MASKS_LEN },
    { "mask_goron",       "Goron Mask",             "One of the four transformation masks.",                                       "", CKI_SPRITE, MSPR_MASK_GORON, CK_SCANEQ, MASKS_BASE, 0x33, MASKS_LEN },
    { "mask_zora",        "Zora Mask",              "One of the four transformation masks.",                                       "", CKI_SPRITE, MSPR_ZORA_MASK, CK_SCANEQ, MASKS_BASE, 0x34, MASKS_LEN },
    { "mask_fierce",      "Fierce Deity's Mask",    "Requires every other mask first.",                                            "", CKI_SPRITE, MSPR_FD_MASK, CK_SCANEQ, MASKS_BASE, 0x35, MASKS_LEN },
    { "mask_truth",       "Mask of Truth",          "Reward for completing every Bombers' Notebook entry.",                        "", CKI_SPRITE, MSPR_MASK_TRUTH, CK_SCANEQ, MASKS_BASE, 0x36, MASKS_LEN },
    { "mask_kafei",       "Kafei's Mask",           "Part of the Anju & Kafei quest.",                                             "", CKI_SPRITE, MSPR_MASK_KAFEI, CK_SCANEQ, MASKS_BASE, 0x37, MASKS_LEN },
    { "mask_allnight",    "All-Night Mask",         "Part of the Romani Ranch quest line.",                                        "", CKI_SPRITE, MSPR_MASK_ALLNIGHT, CK_SCANEQ, MASKS_BASE, 0x38, MASKS_LEN },
    { "mask_bunny",       "Bunny Hood",             "Won from a side quest reward.",                                               "", CKI_SPRITE, MSPR_MASK_BUNNY, CK_SCANEQ, MASKS_BASE, 0x39, MASKS_LEN },
    { "mask_keaton",      "Keaton Mask",            "Reward for correctly answering Keaton's riddles.",                            "", CKI_SPRITE, MSPR_MASK_KEATON, CK_SCANEQ, MASKS_BASE, 0x3A, MASKS_LEN },
    { "mask_garo",        "Garo's Mask",            "Reward from a Garo Master in Ikana Canyon.",                                  "", CKI_SPRITE, MSPR_GARO_MASK, CK_SCANEQ, MASKS_BASE, 0x3B, MASKS_LEN },
    { "mask_romani",      "Romani's Mask",          "Part of the Romani Ranch quest line.",                                        "", CKI_SPRITE, MSPR_MASK_ROMANI, CK_SCANEQ, MASKS_BASE, 0x3C, MASKS_LEN },
    { "mask_circus",      "Troupe Leader's Mask",   "Reward from Gorman for the milk delivery side quest (renamed from OoT/N64's Circus Leader's Mask).", "", CKI_SPRITE, MSPR_MASK_TROUPE, CK_SCANEQ, MASKS_BASE, 0x3D, MASKS_LEN },
    { "mask_postman",     "Postman's Hat",          "Reward for helping the Postman.",                                             "", CKI_SPRITE, MSPR_MASK_POSTMAN, CK_SCANEQ, MASKS_BASE, 0x3E, MASKS_LEN },
    { "mask_couple",      "Couple's Mask",          "Final reward of the Anju & Kafei quest.",                                     "", CKI_SPRITE, MSPR_MASK_COUPLE, CK_SCANEQ, MASKS_BASE, 0x3F, MASKS_LEN },
    { "mask_greatfairy",  "Great Fairy's Mask",     "Reward from the Clock Town Great Fairy for collecting Stray Fairies.",        "", CKI_SPRITE, MSPR_MASK_GREATFAIRY, CK_SCANEQ, MASKS_BASE, 0x40, MASKS_LEN },
    { "mask_gibdo",       "Gibdo Mask",             "Given by the Gibdos in the Ikana royal crypt.",                               "", CKI_SPRITE, MSPR_MASK_GIBDO, CK_SCANEQ, MASKS_BASE, 0x41, MASKS_LEN },
    { "mask_dongero",     "Don Gero's Mask",        "Reward for gathering the frogs at the Woodfall spring.",                      "", CKI_SPRITE, MSPR_MASK_DONGERO, CK_SCANEQ, MASKS_BASE, 0x42, MASKS_LEN },
    { "mask_kamaro",      "Kamaro's Mask",          "Left behind after learning Kamaro's dance.",                                  "", CKI_SPRITE, MSPR_MASK_KAMARO, CK_SCANEQ, MASKS_BASE, 0x43, MASKS_LEN },
    { "mask_captain",     "Captain's Hat",          "Found inside the Pirates' Fortress.",                                         "", CKI_SPRITE, MSPR_MASK_CAPTAIN, CK_SCANEQ, MASKS_BASE, 0x44, MASKS_LEN },
    { "mask_stone",       "Stone Mask",             "Makes most enemies and NPCs ignore you.",                                     "", CKI_SPRITE, MSPR_MASK_STONE, CK_SCANEQ, MASKS_BASE, 0x45, MASKS_LEN },
    { "mask_bremen",      "Bremen Mask",            "Makes young animals march behind you.",                                       "", CKI_SPRITE, MSPR_MASK_BREMEN, CK_SCANEQ, MASKS_BASE, 0x46, MASKS_LEN },
    { "mask_blast",       "Blast Mask",             "Reward from the West Clock Town bomb shop owner.",                            "", CKI_SPRITE, MSPR_MASK_BLAST, CK_SCANEQ, MASKS_BASE, 0x47, MASKS_LEN },
    { "mask_scents",      "Mask of Scents",         "Lets you smell nearby hidden things.",                                        "", CKI_SPRITE, MSPR_MASK_SCENTS, CK_SCANEQ, MASKS_BASE, 0x48, MASKS_LEN },
    { "mask_giant",       "Giant's Mask",           "Turns Link giant-sized - required for the final boss.",                       "", CKI_SPRITE, MSPR_MASK_GIANT, CK_SCANEQ, MASKS_BASE, 0x49, MASKS_LEN },
};

// Stray Fairies (4 dungeons): CONFIRMED address range 0x7763E8-0x7763EB, one byte per dungeon
// holding a 0-15 count (not a bitmask - "All Stray Fairies" fills each with 0x0F = 15). Tracking
// is per-dungeon (15/15), not per-fairy - the save data doesn't expose which specific fairy.
static const ChkItem CK_FAIRIES[] = {
    // key             task                          hint                                                                    loc icon      arg kind        addr      mask
    { "fairy_woodfall", "Woodfall Stray Fairies",    "All 15 collected. MM3D reward: Great Spin Attack (swapped from N64's Snowhead).", "", CKI_SPRITE, MSPR_FAIRY, CK_BYTEEQ, 0x7763E8, 0x0F },
    { "fairy_snowhead", "Snowhead Stray Fairies",    "All 15 collected. MM3D reward: Double Magic Meter (swapped from N64's Woodfall).", "", CKI_SPRITE, MSPR_FAIRY, CK_BYTEEQ, 0x7763E9, 0x0F },
    { "fairy_greatbay", "Great Bay Stray Fairies",   "All 15 collected. Reward: Enhanced Defense.",                          "", CKI_SPRITE, MSPR_FAIRY, CK_BYTEEQ, 0x7763EA, 0x0F },
    { "fairy_ikana",    "Stone Tower Stray Fairies", "All 15 collected. Reward: Great Fairy's Sword.",                       "", CKI_SPRITE, MSPR_FAIRY, CK_BYTEEQ, 0x7763EB, 0x0F },
    { "fairy_clocktown", "Clock Town Stray Fairy",   "The 61st fairy - Laundry Pool by day, Stock Pot Inn area by night. Reward: Great Fairy's Mask.", "", CKI_SPRITE, MSPR_FAIRY, CK_MANUAL, 0, 0 },
};

// Bosses (4): NOT directly save-file confirmed, but decoded by cross-referencing the zeldaret/mm
// N64 decompilation's `QuestItem` enum (include/z64item.h) against our own CONFIRMED 3-byte
// quest field (0x7763D0-0x7763D2, already used by the "All Bosses and Songs" cheat). That enum's
// 24 entries (0x00-0x17) line up exactly with 24 bits across those 3 bytes - byte0=bits 0-7,
// byte1=bits 8-15, byte2=bits 16-23, LSB first. Decoding the CONFIRMED 100%-save value for that
// field (0xCF 0xF7 0xCF) against the enum: bits 0-3 (ODOLWA/GOHT/GYORG/TWINMOLD) are all 1, which
// is exactly what a 100% save should show - strong evidence the bit order/layout guess is right.
// Still not hardware bit-tested, so flagged accordingly - the byte-level "does this equal
// 0xCF/0xF7/0xCF" version of this cheat IS confirmed, individual bits within it aren't.
static const ChkItem CK_BOSSES[] = {
    { "boss_odolwa",   "Odolwa",   "Woodfall Temple.",   "", CKI_SPRITE, MSPR_BOSS_REMAINS, CK_BIT, 0x7763D0, 0x01 },
    { "boss_goht",     "Goht",     "Snowhead Temple.",   "", CKI_SPRITE, MSPR_BOSS_REMAINS, CK_BIT, 0x7763D0, 0x02 },
    { "boss_gyorg",    "Gyorg",    "Great Bay Temple.",  "", CKI_SPRITE, MSPR_BOSS_REMAINS, CK_BIT, 0x7763D0, 0x04 },
    { "boss_twinmold", "Twinmold", "Stone Tower Temple.", "", CKI_SPRITE, MSPR_BOSS_REMAINS, CK_BIT, 0x7763D0, 0x08 },
};

// Heart Pieces (52 = 13 extra Heart Containers). No known save address for individual pieces -
// all manual. MM3D swapped exactly one from the N64 original: Koume's Boat-Cruise Target
// Shooting (Swamp Tourist Center) now gives a Bottle instead of a Heart Piece, and the Dampe
// grave-digging game (Ikana Graveyard, Final Night) gives a Heart Piece instead of a Bottle -
// net count (52) is unchanged, but the LOCATION is, so this list reflects the MM3D placement.
static const ChkItem CK_HEARTS[] = {
    // key       task                          hint                                                                 loc icon      arg kind      addr mask
    { "hp_01", "Deku Flower Deed (South Clock Town)", "Trade the Moon's Tear for the Land Title Deed.",             "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_02", "North Clock Town tree climb",   "Climb the tree using blocks to reach the Bunny Hood platform.",    "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_03", "Swordsman's School",            "Clear the Expert Course, West Clock Town.",                        "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_04", "Deku Scrub Playground",         "Win all 3 days, North Clock Town.",                                "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_05", "Post Office timing game",       "East Clock Town Post Office.",                                     "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_06", "Rosa Sisters dance",            "Dance for them at night wearing Kamaro's Mask, West Clock Town.",  "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_07", "Stock Pot Inn toilet hand",     "Trade the Town Title Deed, Final Night only.",                     "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_08", "Keaton Quiz",                   "Wear Keaton Mask, cut every patch of grass first.",                "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_09", "Mailbox check",                 "Wear the Postman's Hat and check a mailbox.",                     "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_10", "Town Shooting Gallery",         "Perfect run, East Clock Town.",                                    "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_11", "Honey & Darling's",             "Win all 3 days, East Clock Town.",                                 "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_12", "Treasure Chest Shop",           "Win as Goron Link, East Clock Town.",                              "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_13", "Anju's Grandmother, story 1",   "Listen to her first story, wearing the All-Night Mask.",           "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_14", "Anju's Grandmother, story 2",   "Listen to her second story, wearing the All-Night Mask.",          "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_15", "Mayor's Residence meeting",     "Wear the Couple's Mask, end the never-ending meeting.",            "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_16", "Clock Town Bank",               "Deposit 5000 rupees total.",                                       "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_17", "Termina Field grotto",          "Underground hole, Termina Field.",                                 "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_18", "Dodongo grotto",                "Defeat all 3 Dodongos, Termina Field.",                            "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_19", "Business Scrub (Termina Field)", "Buy for 100 rupees near the Astral Observatory.",                 "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_20", "Gossip Stones",                 "Play the right songs at all 4 stones, Termina Field.",             "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_21", "Beehive underwater",            "Bomb the boulder, dive as Zora Link, Termina Field.",              "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_22", "Road to Southern Swamp bats",   "Climb up past the bats and vines.",                                "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_23", "Swamp Deku Flower Deed",        "Trade for the Land Title Deed, Swamp Tourist Center.",             "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_24", "Swamp pictograph contest",      "Photo of the Deku King or Tingle, Swamp Tourist Center.",          "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_25", "Deku Palace maze",              "West Garden maze, Deku Palace.",                                   "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_26", "Woodfall Deku Flower ring",     "Hop the ring of flowers, Woodfall.",                               "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_27", "Swamp Shooting Gallery",        "Perfect run, Road to Southern Swamp.",                             "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_28", "Mountain Deku Flower Deed",     "Trade for the Land Title Deed, Goron Village.",                    "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_29", "Road to Snowhead platforms",    "Invisible/ice platforms - use the Lens of Truth.",                 "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_30", "Underwater chest (Goron Village)", "After Goht is defeated and the river thaws.",                   "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_31", "Frog Choir",                    "Reunite all 5 frogs with Don Gero's Mask, Mountain Village.",      "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_32", "Doggy Racetrack",               "Win 150 rupees in one race, Romani Ranch.",                        "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_33", "Ocean Deku Flower Deed",        "Trade for the Land Title Deed, Zora Hall rooftop area.",           "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_34", "Marine Research Lab tank",      "Feed the tank fish until it grows.",                               "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_35", "Like Like (Zora Cape)",         "Kill it as Zora Link.",                                            "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_36", "Pirates' Fortress switch",      "Hidden switch/chest inside the fortress.",                        "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_37", "Seahorse reunion",              "Reunite the seahorses, Pinnacle Rock.",                            "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_38", "Zora Hall jam session",         "Mikau's diary / join Lulu's band, Evan's song.",                   "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_39", "Beaver Race #2",                "Win the second, faster race, Zora Cape.",                          "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_40", "Great Bay Coast bean ledge",    "Hookshot up to the high ledge, plant a Magic Bean.",               "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_41", "Oceanside Spider House secret", "Fireplace secret room chest.",                                     "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_42", "Fisherman's jumping game",      "Score 20+, after clearing Great Bay Temple.",                      "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_43", "Ikana Deku Flower Deed",        "Trade for the Land Title Deed, Ikana Canyon.",                     "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_44", "Iron Knuckle (Graveyard)",      "Wear the Captain's Hat, Day 1 night, Ikana Graveyard.",            "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_45", "Dampe's grave dig",             "Ikana Graveyard, Final Night only. MM3D: Heart Piece here (was a Bottle in N64).", "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_46", "Poe Sisters",                   "Catch all 4 within the time limit, Beneath the Graveyard.",        "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_47", "Ancient Castle of Ikana roof",  "Rooftop pillar switch.",                                           "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_48", "Secret Shrine",                 "Light Arrow door, defeat all 4 mini-bosses, Ikana Canyon.",        "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_49", "Moon: Deku trial",              "Odolwa's dungeon on the Moon.",                                    "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_50", "Moon: Goron trial",             "Goht's dungeon on the Moon.",                                      "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_51", "Moon: Zora trial",              "Gyorg's dungeon on the Moon.",                                     "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_52", "Moon: Link trial",              "Twinmold's dungeon on the Moon.",                                  "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
};

// Songs (13 in MM3D, one more than N64's 12: Song of Storms is new). 11 of 13 auto-detect via
// the same bit-decoded 0x7763D0-0x7763D2 field as CK_BOSSES above (see that comment for the
// decode methodology and confidence caveat) - Inverted Song of Time and Song of Double Time stay
// manual since they're just the Song of Time played differently, not separately-learned songs
// with their own flag. Note colors are a best-effort match to the real in-game Ocarina Songs
// screen's palette (yellow/gold, red, blue, purple, green, orange badges alongside several
// identical cyan ones) - we could not find a source individually labeling which exact song gets
// which color (unlike Masks, where Zelda Wiki had a named icon file per mask), so the per-song
// color assignment is inferred, not confirmed: Sonata of Awakening=yellow, Goron Lullaby=red,
// New Wave Bossa Nova=blue, Elegy of Emptiness=purple, Oath to Order=green, Song of Storms=
// orange, everything else (Time, Healing, Soaring, Epona's, Scarecrow's, both Time variants)
// stays cyan, matching the majority of the real songs screen.
static const ChkItem CK_SONGS[] = {
    { "song_01", "Song of Time",             "Clock Tower, recovered from Skull Kid.",                    "", CKI_NOTE, 0, CK_BIT, 0x7763D1, 0x10 },
    { "song_02", "Song of Healing",          "Clock Tower, from the Happy Mask Salesman.",                "", CKI_NOTE, 0, CK_BIT, 0x7763D1, 0x20 },
    { "song_03", "Inverted Song of Time",    "Play the Song of Time backwards.",                          "", CKI_NOTE, 0, CK_MANUAL, 0, 0 },
    { "song_04", "Song of Double Time",      "Play the Song of Time doubled.",                            "", CKI_NOTE, 0, CK_MANUAL, 0, 0 },
    { "song_05", "Song of Soaring",          "Kaepora Gaebora, Southern Swamp owl statue.",               "", CKI_NOTE, 0, CK_BIT, 0x7763D1, 0x80 },
    { "song_06", "Sonata of Awakening",      "From the monkey, Deku Palace.",                             "", CKI_NOTE, 6, CK_BIT, 0x7763D0, 0x40 },
    { "song_07", "Goron Lullaby",            "Goron Elder + the crying baby, Goron Village.",             "", CKI_NOTE, 2, CK_BIT, 0x7763D0, 0x80 },
    { "song_08", "Epona's Song",             "From Romani, Romani Ranch.",                                "", CKI_NOTE, 0, CK_BIT, 0x7763D1, 0x40 },
    { "song_09", "New Wave Bossa Nova",      "The Zora Eggs, Marine Research Lab.",                       "", CKI_NOTE, 3, CK_BIT, 0x7763D1, 0x01 },
    { "song_10", "Elegy of Emptiness",       "Igos du Ikana, Ancient Castle of Ikana.",                   "", CKI_NOTE, 5, CK_BIT, 0x7763D1, 0x02 },
    { "song_11", "Oath to Order",            "Odolwa's Giant, after clearing Woodfall Temple.",           "", CKI_NOTE, 1, CK_BIT, 0x7763D1, 0x04 },
    { "song_12", "Scarecrow's Song",         "Teach it to Pierre (Trading Post / Astral Observatory).",  "", CKI_NOTE, 0, CK_BIT, 0x7763D2, 0x02 },
    { "song_13", "Song of Storms",           "MM3D-EXCLUSIVE. Beneath the Graveyard, after Flat's eulogy + Iron Knuckle; needs Captain's Hat.", "", CKI_NOTE, 4, CK_BIT, 0x7763D2, 0x01 },
};

// Bomber's Notebook: MM3D overhauled this to 63 trackable events (the N64 original only tracked
// 20 people). Completing all 63 puts a ribbon on the notebook header. No known save address -
// all manual.
static const ChkItem CK_NOTEBOOK[] = {
    { "note_01", "A Stay at Stock Pot Inn",       "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_02", "Anju's Anguish",                "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_03", "A Testament of Love",           "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_04", "The Never-Ending Meeting",      "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_05", "Madame Aroma's Search",         "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_06", "A Challenge to Count On",       "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_07", "The Postman's Peril",           "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_08", "Curiosity Shop Rarity",         "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_09", "The Bomb Business",             "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_10", "History of the Carnival",       "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_11", "Termina Mythology",             "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_12", "The Ghost of the Inn",          "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_13", "A Melancholy Melody",           "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_14", "A Dance with Meaning",          "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_15", "Music Moves the Heart",         "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_16", "A Race near Milk Road",         "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_17", "Protect Romani's Cows!",        "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_18", "Protect the Milk!",             "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_19", "Cucco Shack's Cute Chicks",     "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_20", "Find the Stone-Faced Soldier",  "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_21", "Great Fairy of Clock Town",     "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_22", "Great Fairy of the Swamp",      "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_23", "Great Fairy of the Mountains",  "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_24", "Great Fairy of the Ocean",      "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_25", "Great Fairy of the Canyon",     "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_26", "Business Scrub Scramble",       "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_27", "Bank Loyalty Program",          "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_28", "The Suspicious Ocean House",    "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_29", "Target-Shooting Champ",         "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_30", "Swamp Shooting Champ",          "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_31", "Three Days of Gaming",          "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_32", "Lucky Numbers",                 "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_33", "Master Swordsman",              "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_34", "A Treasure-Chest Prize",        "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_35", "Deku Flower Power",             "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_36", "Find a Keaton!",                "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_37", "Secret Gossip",                 "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_38", "Follow That Scrub!",            "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_39", "Pictograph Contest",            "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_40", "The Terrifying Swamp House",    "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_41", "A Potion Hag's New Business",   "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_42", "A Royal Rush",                  "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_43", "A Goron's Grief",               "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_44", "An Explosive Exam",             "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_45", "Goron Races! Rock 'n' Roll!",   "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_46", "A Sharper Sword",               "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_47", "Reunite the Frog Choir",        "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_48", "Win Big at the Doggy Race",     "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_49", "Fishy Friends",                 "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_50", "Spider House Mystery",          "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_51", "A Fish Wish",                   "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_52", "Gimme a Break",                 "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_53", "Race the Beaver Bros.!",        "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_54", "Light It to Right It",          "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_55", "Playing Paparazzi",             "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_56", "A Zora Swan Song",              "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_57", "The Seafarer's Challenge",      "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_58", "Buried Treasure",               "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_59", "Free the Canyon Ghosts",        "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_60", "Vanquished Foes Return",        "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_61", "The Bombers' Code",             "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_62", "Child's Play",                  "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_63", "Fraternal Milk",                "MM3D-EXCLUSIVE quest, also the source of the game's 7th bottle.", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
};

// Owl Statues (10, save points - checked, not slashed, in MM3D, and they save permanently since
// Song of Time no longer erases owl saves here). No known save address - all manual.
static const ChkItem CK_OWLS[] = {
    { "owl_01", "South Clock Town",     "By the Bank (relocated from beside the Clock Tower in N64).", "", CKI_SPRITE, MSPR_OWL_ICON, CK_MANUAL, 0, 0, 0, MSPR_OWL_MODEL },
    { "owl_02", "Milk Road",            "Termina Field, at the Milk Road entrance.",                    "", CKI_SPRITE, MSPR_OWL_ICON, CK_MANUAL, 0, 0, 0, MSPR_OWL_MODEL },
    { "owl_03", "Southern Swamp",       "Outside the Swamp Tourist Center.",                            "", CKI_SPRITE, MSPR_OWL_ICON, CK_MANUAL, 0, 0, 0, MSPR_OWL_MODEL },
    { "owl_04", "Woodfall",             "In front of Woodfall Temple.",                                 "", CKI_SPRITE, MSPR_OWL_ICON, CK_MANUAL, 0, 0, 0, MSPR_OWL_MODEL },
    { "owl_05", "Mountain Village",     "Near the smithy.",                                             "", CKI_SPRITE, MSPR_OWL_ICON, CK_MANUAL, 0, 0, 0, MSPR_OWL_MODEL },
    { "owl_06", "Snowhead",             "On the path before Snowhead Temple.",                          "", CKI_SPRITE, MSPR_OWL_ICON, CK_MANUAL, 0, 0, 0, MSPR_OWL_MODEL },
    { "owl_07", "Great Bay Coast",      "Near the Marine Research Lab.",                                "", CKI_SPRITE, MSPR_OWL_ICON, CK_MANUAL, 0, 0, 0, MSPR_OWL_MODEL },
    { "owl_08", "Zora Cape",            "Outside Zora Hall.",                                           "", CKI_SPRITE, MSPR_OWL_ICON, CK_MANUAL, 0, 0, 0, MSPR_OWL_MODEL },
    { "owl_09", "Ikana Canyon",         "Atop the cliff past the broken bridge.",                       "", CKI_SPRITE, MSPR_OWL_ICON, CK_MANUAL, 0, 0, 0, MSPR_OWL_MODEL },
    { "owl_10", "Stone Tower",          "At the Stone Tower Temple entrance.",                          "", CKI_SPRITE, MSPR_OWL_ICON, CK_MANUAL, 0, 0, 0, MSPR_OWL_MODEL },
};

// Bottles (7 in MM3D, one more than N64's 6 - two changes net one extra: Koume's Boat-Cruise
// archery gives a Bottle here instead of N64's Heart Piece, AND the Gorman "Fraternal Milk"
// sidequest is entirely new to MM3D). No known save address for "which bottles you've unlocked"
// (as opposed to bottle CONTENTS, which the Bottle #1-7 pickers already read/write) - all manual.
static const ChkItem CK_BOTTLES[] = {
    { "bottle_01", "Kotake's Red Potion",       "Woods of Mystery, after saving Koume.",                              "", CKI_SPRITE, MSPR_B_REDPOTION, CK_MANUAL, 0, 0 },
    { "bottle_02", "Koume's Boat-Cruise",       "Target shooting, 20+ points, Swamp Tourist Center. MM3D: Bottle here (was a Heart Piece in N64).", "", CKI_SPRITE, MSPR_B_HOTSPRING, CK_MANUAL, 0, 0 }, // playful stand-in: no bottle art fits an archery minigame
    { "bottle_03", "Chateau Romani",            "Survive the alien night with Romani, Romani Ranch.",                 "", CKI_SPRITE, MSPR_B_CHATEAU, CK_MANUAL, 0, 0 },
    { "bottle_04", "Gold Dust",                 "Win the Goron Racetrack after defeating Goht.",                      "", CKI_SPRITE, MSPR_B_GOLDDUST, CK_MANUAL, 0, 0 },
    { "bottle_05", "Fraternal Milk",             "MM3D-EXCLUSIVE. Gorman + Troupe Leader's Mask, fetch within 2 minutes, Stock Pot Inn/Milk Road.", "", CKI_SPRITE, MSPR_B_MILK, CK_MANUAL, 0, 0 },
    { "bottle_06", "Beaver Brothers race #1",   "Win the first race, Zora Cape waterfall cave.",                      "", CKI_SPRITE, MSPR_B_FISH, CK_MANUAL, 0, 0 }, // playful stand-in: a fish for a waterfall-cave swimming race
    { "bottle_07", "Madame Aroma's mail",       "Deliver Priority Mail with Kafei's Mask, Milk Bar.",                 "", CKI_SPRITE, MSPR_BOTTLE, CK_MANUAL, 0, 0 }, // playful stand-in: "message in a bottle" for a mail delivery
};

// Equipment upgrades. The first four already have CONFIRMED cheat addresses elsewhere in this
// plugin (Battle/Inventory folders) and auto-detect from them; the rest have no known address
// and are manual. Great Fairy's Sword is a C-item, not a sword-slot upgrade - tracked here
// separately from the 3 sword tiers. Wallets are 3 tiers total in MM(3D), not 4.
static const ChkItem CK_EQUIP[] = {
    // auto-detected (confirmed addresses)
    { "eq_gilded",   "Gilded Sword",           "Smithy + Gold Dust. Final sword tier.",                    "", CKI_SPRITE, MSPR_SWORD, CK_BYTEEQ,  0x776352, 0x23 },
    { "eq_defense",  "Enhanced Defense",       "Great Bay Stray Fairy reward. Halves damage taken.",       "", CKI_SPRITE, MSPR_DEFENSE, CK_NONZERO, 0x776320, 0x00 },
    { "eq_magic",    "Magic Meter",            "Unlocks the magic bar.",                                   "", CKI_SPRITE, MSPR_MAGIC_FAIRY, CK_NONZERO, 0x77631E, 0x00 },
    { "eq_dmagic",   "Double Magic",           "Snowhead Stray Fairy reward (MM3D). Doubles magic capacity.", "", CKI_SPRITE, MSPR_MAGIC_FAIRY, CK_NONZERO, 0x77631F, 0x00 },
    // manual (no known address)
    { "eq_razor",    "Razor Sword",            "Mountain Smithy, 100 rupees, wait until morning. Reverts on Song of Time.", "", CKI_SPRITE, MSPR_EQ_RAZOR, CK_MANUAL, 0, 0 },
    { "eq_hero",     "Hero's Shield",          "Starting shield.",                                         "", CKI_SPRITE, MSPR_EQ_HERO, CK_MANUAL, 0, 0 },
    { "eq_mirror",   "Mirror Shield",          "Big chest, Beneath the Well (Ikana).",                     "", CKI_SPRITE, MSPR_DEFENSE, CK_MANUAL, 0, 0 },
    { "eq_gfsword",  "Great Fairy's Sword",    "Stone Tower Stray Fairy reward. A C-item, not a sword upgrade.", "", CKI_SPRITE, MSPR_GFSWORD, CK_MANUAL, 0, 0 },
    { "eq_quiver1",  "Quiver (30)",            "With the Hero's Bow, Woodfall Temple.",                    "", CKI_SPRITE, MSPR_QUIVER, CK_MANUAL, 0, 0 },
    { "eq_quiver2",  "Large Quiver (40)",      "Town Shooting Gallery, 40+ points.",                       "", CKI_SPRITE, MSPR_EQ_QUIVER2, CK_MANUAL, 0, 0 },
    { "eq_quiver3",  "Largest Quiver (50)",    "Swamp Shooting Gallery, perfect run.",                     "", CKI_SPRITE, MSPR_EQ_QUIVER3, CK_MANUAL, 0, 0 },
    { "eq_bombbag1", "Bomb Bag (20)",          "Bomb Shop, 50 rupees.",                                    "", CKI_SPRITE, MSPR_EQ_BOMBBAG1, CK_MANUAL, 0, 0 },
    { "eq_bombbag2", "Big Bomb Bag (30)",      "Bomb Shop, 90 rupees, after saving the old lady Night 1.", "", CKI_SPRITE, MSPR_EQ_BOMBBAG2, CK_MANUAL, 0, 0 },
    { "eq_bombbag3", "Biggest Bomb Bag (40)",  "Goron Village Business Scrub, Big Bomb Bag + 200 rupees.", "", CKI_SPRITE, MSPR_EQ_BOMBBAG3, CK_MANUAL, 0, 0 },
    { "eq_wallet1",  "Adult Wallet (200)",     "Clock Town Bank, deposit 200 rupees.",                     "", CKI_SPRITE, MSPR_WALLET, CK_MANUAL, 0, 0 },
    { "eq_wallet2",  "Giant Wallet (500)",     "Oceanside Spider House, all 30 Gold Skulltulas (any day in MM3D).", "", CKI_SPRITE, MSPR_EQ_WALLET2, CK_MANUAL, 0, 0 },
};

static const ChkCat CHK_CATS[] = {
    { "Masks",            CK_MASKS,    (int)(sizeof(CK_MASKS)    / sizeof(CK_MASKS[0])) },
    { "Heart Pieces",     CK_HEARTS,   (int)(sizeof(CK_HEARTS)   / sizeof(CK_HEARTS[0])) },
    { "Songs",            CK_SONGS,    (int)(sizeof(CK_SONGS)    / sizeof(CK_SONGS[0])) },
    { "Bosses",           CK_BOSSES,   (int)(sizeof(CK_BOSSES)   / sizeof(CK_BOSSES[0])) },
    { "Stray Fairies",    CK_FAIRIES,  (int)(sizeof(CK_FAIRIES)  / sizeof(CK_FAIRIES[0])) },
    { "Bomber's Notebook", CK_NOTEBOOK, (int)(sizeof(CK_NOTEBOOK) / sizeof(CK_NOTEBOOK[0])) },
    { "Owl Statues",       CK_OWLS,    (int)(sizeof(CK_OWLS)     / sizeof(CK_OWLS[0])) },
    { "Bottles",           CK_BOTTLES, (int)(sizeof(CK_BOTTLES)  / sizeof(CK_BOTTLES[0])) },
    { "Equipment",         CK_EQUIP,   (int)(sizeof(CK_EQUIP)    / sizeof(CK_EQUIP[0])) },
};
#define CHK_NCATS  ((int)(sizeof(CHK_CATS) / sizeof(CHK_CATS[0])))
#define CHK_MAXITEMS 63
#define CHK_LEAF "Tracker.txt"

static u8  chkState[CHK_NCATS][CHK_MAXITEMS]; // 0 untouched, 1 auto, 2 you-checked, 3 you-cleared
static int chkLoaded = 0;

// ---- type-icons: 8x8 monochrome bitmaps, scaled to any pixel size (placeholders; real sprites
// swap in real art later by adding a sprite kind - see DrawChkIcon) ----
static const u8 iconHeartBmp[8] = { 0x66,0xFF,0xFF,0xFF,0x7E,0x3C,0x18,0x00 };
static const u8 iconSkullBmp[8] = { 0x24,0x3C,0x7E,0xFF,0xFF,0x7E,0x3C,0x42 };
static const u8 iconNoteBmp[8]  = { 0x0C,0x0C,0x0E,0x0C,0x0C,0x7C,0xFC,0x78 };
static const u8 iconKeyBmp[8]   = { 0x38,0x44,0x44,0x38,0x10,0x10,0x34,0x00 };
static void DrawBitmapIcon(const u8 *bmp, int x, int y, int cell, u8 r, u8 g, u8 b)
{
    for (int row = 0; row < 8; ++row)
        for (int col = 0; col < 8; ++col)
            if (bmp[row] & (0x80 >> col))
                CFill(x + col * cell, y + row * cell, cell, cell, r, g, b);
}
// cell=2 -> 16px (list rows, matches DrawSprite's small size); cell=5 -> 40px (detail card).
static void DrawChkIcon(const ChkItem *it, int x, int y, int cell)
{
    switch (it->iconKind)
    {
        case CKI_SPRITE: {
            // Detail card (cell > 3) uses iconArgBig if this item has one (Owl Statues' bigger
            // 3D-render crop); every other item and every list row uses the one iconArg key.
            int key = (cell > 3 && it->iconArgBig) ? it->iconArgBig : it->iconArg;
            const SpriteRef *s = FindSprite(key);
            if (s) DrawScaled(x, y, cell * 8, cell * 8, s->px16, SPR16, SPR16, 0);
            break;
        }
        case CKI_SWATCH: {
            u16 c = it->iconArg;
            u8 r = (u8)(((c >> 11) & 0x1F) * 255 / 31), g = (u8)(((c >> 5) & 0x3F) * 255 / 63), b = (u8)((c & 0x1F) * 255 / 31);
            CFill(x, y, cell * 8, cell * 8, r, g, b);
            break;
        }
        case CKI_HEART:   DrawBitmapIcon(iconHeartBmp, x, y, cell, 220, 60, 60); break;
        case CKI_SKULL:   DrawBitmapIcon(iconSkullBmp, x, y, cell, 224, 186, 96); break;
        case CKI_NOTE: {
            // A palette of note colors; iconArg picks one. Handy for any "collect the set" list:
            // 0 = cyan (the 4 non-warp songs are all cyan) then the 6 warp songs by their color.
            static const u8 songCol[7][3] = {
                { 90,210,230}, // 0 cyan
                { 90,210, 90}, // 1 green - Minuet of Forest
                {230, 90, 90}, // 2 red   - Bolero of Fire
                { 96,128,235}, // 3 blue  - Serenade of Water
                {235,160, 70}, // 4 orange- Requiem of Spirit
                {184,102,222}, // 5 purple- Nocturne of Shadow
                {232,212, 84}, // 6 yellow- Prelude of Light
            };
            int ci = it->iconArg < 7 ? it->iconArg : 0;
            DrawBitmapIcon(iconNoteBmp, x, y, cell, songCol[ci][0], songCol[ci][1], songCol[ci][2]);
            break;
        }
        case CKI_KEYITEM: DrawBitmapIcon(iconKeyBmp,   x, y, cell, 180, 150, 90); break;
        default: break;
    }
}

// Small-font clip-with-ellipsis (CTextClip's sibling for CText6). Reusable beyond the checklist.
static void CText6Clip(int x, int y, const char *s, int maxw, u8 r, u8 g, u8 b)
{
    char buf[64]; int n = 0;
    while (s[n] && n < 62) { buf[n] = s[n]; n++; }
    buf[n] = 0;
    if (C6Width(buf) <= maxw) { CText6(x, y, buf, r, g, b); return; }
    while (n > 1)
    {
        buf[--n] = 0;
        char tmp[66]; int t = 0;
        for (int i = 0; i < n; ++i) tmp[t++] = buf[i];
        tmp[t++] = '.'; tmp[t++] = '.'; tmp[t] = 0;
        if (C6Width(tmp) <= maxw) { CText6(x, y, tmp, r, g, b); return; }
    }
    CText6(x, y, "..", r, g, b);
}
// Clipped small-font draw used by the marquee to paint only inside [clipX0, clipX1).
static void CText6ClipRegion(int x, int y, const char *s, int clipX0, int clipX1, u8 r, u8 g, u8 b)
{
    while (*s)
    {
        unsigned char ch = SmallAscii(SysFontUtf8Next(&s));
        if (ch)
        {
            const unsigned char *glyph = &font[ch * FONT_HEIGHT];
            for (int dy = 0; dy < FONT_HEIGHT; ++dy)
                for (int dx = 0; dx < FONT_WIDTH; ++dx)
                    if (glyph[dy] & (0x80 >> dx))
                    {
                        int X = x + dx, Y = y + dy;
                        if (X < clipX0 || X >= clipX1) continue;
                        if ((unsigned)X < TOP_W && (unsigned)Y < TOP_H)
                        { u8 *p = CPix(X, Y); p[0] = r; p[1] = g; p[2] = b; }
                    }
        }
        x += FONT_WIDTH + 1;
    }
}
// Selected-row marquee: static (clipped) if it fits or the hold delay hasn't elapsed yet, else
// scrolls left and loops with a gap. `delay` is in frames (~16ms each) and is also the moment
// the scroll animation itself starts counting from, so it always starts smoothly at offset 0.
#define CHK_MARQUEE_DELAY      62    // ~1s - list rows
#define CHK_HINT_MARQUEE_DELAY 124   // ~2s - the item-card Hint field
#define CHK_SPEED_LIST 3, 2 // 1.5x  - list rows (task name)
#define CHK_SPEED_FAST 9, 4 // 2.25x - Hint/Where (1.5x on top of the 1.5x list speed)
static void CText6Marquee(int x, int y, int w, const char *s, int tick, int delay, int spdNum, int spdDen, u8 r, u8 g, u8 b)
{
    int tw = C6Width(s);
    if (tw <= w || tick < delay) { CText6Clip(x, y, s, w, r, g, b); return; }
    int cyclepx = tw + 24;
    int off = ((tick - delay) * spdNum / spdDen) % cyclepx;
    CText6ClipRegion(x - off, y, s, x, x + w, r, g, b);
    CText6ClipRegion(x - off + cyclepx, y, s, x, x + w, r, g, b);
}
// Wraps a short label into up to 2 lines within width w, breaking on spaces. A lone "&" is
// glued to the word that follows it BEFORE wrapping, so a line never ends on a dangling "&" -
// used for hub category names ("Zora's Domain & Jabu-Jabu" etc).
//
// Split choice is BALANCED, not greedy: if the whole label fits on one line, it stays on one
// line. Otherwise, among every point where it could break into 2 lines, pick the one that
// minimizes the longer of the two resulting line widths. A naive "pack line1 as full as
// possible" wrap strands short connector words ("an", "&") alone on line1 just because there
// happened to be room, e.g. "Becoming an" / "Adult" instead of the more even "Becoming" /
// "an Adult"; balancing avoids that.
typedef int (*ChkMeasureFn)(const char *);
static void ChkWrapBalanced(const char *s, int w, char *line1, char *line2, ChkMeasureFn measure)
{
    char buf[64]; int n = 0;
    while (s[n] && n < 62) { buf[n] = s[n]; n++; }
    buf[n] = 0;

    char *tok[12]; int ntok = 0;
    char *p = buf;
    while (*p && ntok < 12)
    {
        while (*p == ' ') *p++ = 0;
        if (!*p) break;
        tok[ntok++] = p;
        while (*p && *p != ' ') p++;
    }
    char *mtok[12]; int nmtok = 0;
    for (int i = 0; i < ntok; ++i)
    {
        if (strcmp(tok[i], "&") == 0 && i + 1 < ntok)
        { *(tok[i + 1] - 1) = ' '; mtok[nmtok++] = tok[i]; ++i; } // "&" + next word -> one token
        else mtok[nmtok++] = tok[i];
    }

    line1[0] = 0; line2[0] = 0;
    if (nmtok == 0) return;

    char joined[64]; joined[0] = 0;
    for (int i = 0; i < nmtok; ++i)
    { char c[64]; if (joined[0]) sniprintf(c, sizeof c, "%s %s", joined, mtok[i]); else sniprintf(c, sizeof c, "%s", mtok[i]); strcpy(joined, c); }
    if (measure(joined) <= w) { strcpy(line1, joined); return; } // fits on one line - don't split it

    int bestSplit = -1, bestMax = 0x7FFFFFFF;
    char cur[64]; cur[0] = 0;
    for (int k = 0; k < nmtok; ++k)
    {
        char cand[64];
        if (cur[0]) sniprintf(cand, sizeof cand, "%s %s", cur, mtok[k]); else sniprintf(cand, sizeof cand, "%s", mtok[k]);
        strcpy(cur, cand);
        int w1 = measure(cur);
        if (w1 > w) break; // line1 can't extend this far and still fit
        int w2 = 0;
        if (k + 1 < nmtok)
        {
            char rest[64]; rest[0] = 0;
            for (int j = k + 1; j < nmtok; ++j)
            { char c2[64]; if (rest[0]) sniprintf(c2, sizeof c2, "%s %s", rest, mtok[j]); else sniprintf(c2, sizeof c2, "%s", mtok[j]); strcpy(rest, c2); }
            w2 = measure(rest);
        }
        int m = w1 > w2 ? w1 : w2;
        if (m < bestMax) { bestMax = m; bestSplit = k + 1; }
    }
    if (bestSplit < 0) bestSplit = 1; // even the first token alone overflows - force it anyway

    for (int i = 0; i < bestSplit; ++i)
    { char c[64]; if (line1[0]) sniprintf(c, sizeof c, "%s %s", line1, mtok[i]); else sniprintf(c, sizeof c, "%s", mtok[i]); strcpy(line1, c); }
    for (int i = bestSplit; i < nmtok; ++i)
    { char c[64]; if (line2[0]) sniprintf(c, sizeof c, "%s %s", line2, mtok[i]); else sniprintf(c, sizeof c, "%s", mtok[i]); strcpy(line2, c); }
}
#define CHK_BIGLINE_H 13 // stacked-line pitch for 2-line system-font labels (hub grid/buttons)
// Left-aligned 2-line wrap, system font, vertically centered within box height h (paired with a
// right-aligned count next to it, e.g. hub top screen). h-centering matters: a selection
// highlight covers the full row - if this always assumed 1 line, a 2-line label's 2nd line
// would spill out past the highlight instead of sitting inside it.
static void CTextWrap2(int x, int y, int w, int h, const char *s, u8 r, u8 g, u8 b)
{
    char line1[64], line2[64];
    ChkWrapBalanced(s, w, line1, line2, CTextWidth);
    int blockH = line2[0] ? (CHK_BIGLINE_H * 2) : CHK_BIGLINE_H;
    int ly = y + (h - blockH) / 2;
    CText(x, ly, line1, r, g, b, 0);
    if (line2[0]) CTextClip(x, ly + CHK_BIGLINE_H, line2, w, r, g, b, 0);
}
// 2-line wrap, system font, centered on BOTH axes inside a box (x,y,w,h) - e.g. touch buttons,
// so a 1-line name sits in the middle of the box instead of stuck to the top like a 2-line one.
static void CTextWrap2CenterBox(int x, int y, int w, int h, const char *s, u8 r, u8 g, u8 b)
{
    char line1[64], line2[64];
    ChkWrapBalanced(s, w, line1, line2, CTextWidth);
    int blockH = line2[0] ? (CHK_BIGLINE_H * 2) : CHK_BIGLINE_H;
    int ly = y + (h - blockH) / 2;
    CText(x + (w - CTextWidth(line1)) / 2, ly, line1, r, g, b, 0);
    if (line2[0])
    {
        int lw2 = CTextWidth(line2), ly2 = ly + CHK_BIGLINE_H;
        if (lw2 <= w) CText(x + (w - lw2) / 2, ly2, line2, r, g, b, 0);
        else          CTextClip(x, ly2, line2, w, r, g, b, 0);
    }
}

static int ChkFindKey(const char *key, int *outC, int *outI)
{
    for (int c = 0; c < CHK_NCATS; ++c)
        for (int i = 0; i < CHK_CATS[c].count; ++i)
            if (strcmp(CHK_CATS[c].items[i].key, key) == 0) { *outC = c; *outI = i; return 1; }
    return 0;
}
static void ChecklistSave(void)
{
    FsBootInit(); if (!fsReady) return;
    Handle f;
    if (R_FAILED(FSUSER_OpenFile(&f, cfgArchive, fsMakePath(PATH_ASCII, PlgPath(CHK_LEAF)),
                                 FS_OPEN_WRITE | FS_OPEN_CREATE, 0)))
        return;
    char buf[80]; u32 off = 0, wrote;
    static const char *hdr = "# CTRComposer tracker state. Keyed by item id, so it survives edits.\nVER 1\n";
    FSFILE_Write(f, &wrote, off, hdr, (u32)strlen(hdr), FS_WRITE_FLUSH); off += wrote;
    for (int c = 0; c < CHK_NCATS; ++c)
        for (int i = 0; i < CHK_CATS[c].count; ++i)
        {
            u8 s = chkState[c][i]; if (!s) continue;
            const char *tag = (s == 1) ? "A" : (s == 2) ? "M" : "S";
            int n = sniprintf(buf, sizeof buf, "STATE %s %s\n", CHK_CATS[c].items[i].key, tag);
            if (n > (int)sizeof buf - 1) n = (int)sizeof buf - 1;
            FSFILE_Write(f, &wrote, off, buf, (u32)n, FS_WRITE_FLUSH); off += wrote;
        }
    FSFILE_SetSize(f, off);
    FSFILE_Close(f);
}
// Wipe every item back to "not done" and rewrite Checklist.txt (bound to START on the hub).
static void ChecklistReset(void)
{
    for (int c = 0; c < CHK_NCATS; ++c)
        for (int i = 0; i < CHK_CATS[c].count; ++i)
            chkState[c][i] = 0;
    ChecklistSave();
}
static void ChecklistLoad(void)
{
    chkLoaded = 1;
    memset(chkState, 0, sizeof(chkState));
    FsBootInit(); if (!fsReady) return;
    Handle f;
    if (R_FAILED(FSUSER_OpenFile(&f, cfgArchive, fsMakePath(PATH_ASCII, PlgPath(CHK_LEAF)), FS_OPEN_READ, 0)))
        return;
    u64 sz64 = 0; FSFILE_GetSize(f, &sz64);
    u32 sz = (u32)sz64;
    if (sz == 0 || sz > 64 * 1024) { FSFILE_Close(f); return; }
    char *buf = (char *)malloc(sz + 1);
    if (!buf) { FSFILE_Close(f); return; }
    u32 got = 0;
    Result r = FSFILE_Read(f, &got, 0, buf, sz);
    FSFILE_Close(f);
    if (R_FAILED(r) || !got) { free(buf); return; }
    buf[got] = 0;
    char *p = buf;
    while (*p)
    {
        char *line = p;
        while (*p && *p != '\n') p++;
        char *eol = p; if (*p) p++;
        if (eol > line && eol[-1] == '\r') eol[-1] = 0;
        *eol = 0;
        if (!strncmp(line, "STATE ", 6))
        {
            char *key = line + 6;
            char *sp = strchr(key, ' ');
            if (sp)
            {
                *sp = 0; char *tag = sp + 1;
                int c, i;
                if (ChkFindKey(key, &c, &i))
                {
                    u8 v = (tag[0] == 'A') ? 1 : (tag[0] == 'M') ? 2 : (tag[0] == 'S') ? 3 : 0;
                    if (v) chkState[c][i] = v;
                }
            }
        }
    }
    free(buf);
}
static void ChecklistLoadOnce(void) { if (!chkLoaded) ChecklistLoad(); }

// Auto-fill = SYNC the auto-marks to the CURRENTLY loaded save (so switching to a lesser save no
// longer keeps a previous save's marks). For each item it reads live gSaveContext:
//   detected  -> set state 1 (auto), but only from 0/3 - never overrides 2 (you manually checked)
//   NOT detected -> clear state 1 (stale auto) back to 0, but leave 2/3 (your own tracking) alone
// Detections are false-positive-proof, so re-marking a 3 (you-cleared) that the save confirms is safe.
// g_afAdd / g_afRem hold the last run's added / removed counts for the on-screen result.
static int g_afAdd, g_afRem;
static int ChecklistAutoFill(void)
{
    g_afAdd = g_afRem = 0;
    for (int c = 0; c < CHK_NCATS; ++c)
        for (int i = 0; i < CHK_CATS[c].count; ++i)
        {
            const ChkItem *it = &CHK_CATS[c].items[i];
            int got = 0;
            // addr 0 means "not wired up yet" (the shipped examples) - never dereference it.
            if (it->kind == CK_MANUAL || !it->addr) continue; // never auto-touch these
            if (it->kind == CK_BIT)          got = (R8(it->addr) & it->mask) != 0;
            else if (it->kind == CK_BYTEEQ)  got = R8(it->addr) == it->mask;
            else if (it->kind == CK_NONZERO) got = R8(it->addr) != 0;
            else if (it->kind == CK_SCANEQ)
            {
                for (int o = 0; o < it->scanLen; ++o)
                    if (R8(it->addr + o) == it->mask) { got = 1; break; }
            }
            else continue;
            u8 st = chkState[c][i];
            if (got) { if (st == 0 || st == 3) { chkState[c][i] = 1; ++g_afAdd; } }
            else     { if (st == 1)            { chkState[c][i] = 0; ++g_afRem; } }
        }
    return g_afAdd + g_afRem;
}
// Build the "+A -R" / "Up to date" result string for the auto-fill button.
static void ChkAfMsg(char *buf, const char **msg)
{
    if (g_afAdd || g_afRem) { siprintf(buf, "+%d -%d", g_afAdd, g_afRem); *msg = buf; }
    else *msg = "Up to date";
}

// Bottom screen is 320px wide, NOT the top window's 360 - every x below is native to that.
#define CHKB_L    8
#define CHKB_R    312
#define CHKB_COLW 148
// Hub grid is paginated 2 cols x 4 rows (8 categories/page) - a flat unpaginated grid stopped
// fitting once the dataset grew past a couple of test areas, and 2-line names need more row
// height than the old 1-line design had room for.
#define HUB_COLS   2
#define HUB_ROWS   4
#define HUB_PAGESZ (HUB_COLS * HUB_ROWS)
#define CHKB_BTN_H  36
#define CHKB_STRIDE 40
#define CHKB_TOP    10
// Marquee text-box widths, mirrored from the draw code below (task row / hint & where pill) -
// duplicated here so the pre-draw "is anything actually scrolling" check can be computed
// without doing a full draw pass.
#define CHK_TASK_TXTW ((CHKB_R - 16) - 6 - (CHKB_L + 16 + 8))
#define CHK_HINT_SVW  (WIN_X + WIN_W - 12 - (WIN_X + 86))
static void ToolChecklist(void)
{
    static int level = 0;             // 0 hub, 1 item list (persists across SELECT/reopen)
    static int catCur = 0;
    static int catCursor[CHK_NCATS], catScroll[CHK_NCATS];
    static int filterMode = 0;        // 0 All, 1 Todo, 2 Done
    static int revealLoc = 0;         // X toggle, sticky for the session
    static int selTick = 0;           // frames on the current item -> drives its marquee delay
    static const char *afMsg = NULL;  // Auto-fill result shown on the button (right side), timed
    static int afTick = 0;            // countdown for afMsg
    static char afBuf[24];
    ChecklistLoadOnce();
    if (cheatState[CH_CFG_AUTOFILL]) // auto-fill from the save every time the Checklist opens (Settings toggle)
    {
        ChecklistAutoFill(); ChecklistSave(); ChkAfMsg(afBuf, &afMsg);
        afTick = 180;
    }
    KbInit(); // touch service - every other touch-using tool calls this too

    int touchPrev = HidTouch(0, 0);
    u32 prev = HID_PAD;
    int redraw = 1;
    int lastCur = -1000;
    int hubPages = (CHK_NCATS + HUB_PAGESZ - 1) / HUB_PAGESZ;

    while (1)
    {
        int px, py, nowT = HidTouch(&px, &py);
        int tap = nowT && !touchPrev; touchPrev = nowT;
        if (!hidReady) tap = 0;
        svcSleepThread(16 * 1000 * 1000);
        u32 pad = HID_PAD, down = ARepeat(pad, &prev, &g_arHold);
        int selOverflow = 0; // is a marquee on the CURRENT item actually mid-scroll right now?

        if (down & BUTTON_SELECT) { g_quitToGame = 1; return; } // resumes at this level/cat/cursor

        if (level == 0)
        {
            if (down & BUTTON_B) return;
            int page = catCur / HUB_PAGESZ, local = catCur % HUB_PAGESZ;
            int pageBase = page * HUB_PAGESZ;
            int pageN = CHK_NCATS - pageBase; if (pageN > HUB_PAGESZ) pageN = HUB_PAGESZ;
            int pageRows = (pageN + HUB_COLS - 1) / HUB_COLS;
            int col = local % HUB_COLS, row = local / HUB_COLS;
            if (down & BUTTON_DOWN)
            { row = (row + 1) % pageRows; int ni = col + row * HUB_COLS;
              catCur = pageBase + ((ni < pageN) ? ni : (row * HUB_COLS < pageN ? row * HUB_COLS : pageN - 1)); redraw = 1; }
            if (down & BUTTON_UP)
            { row = (row - 1 + pageRows) % pageRows; int ni = col + row * HUB_COLS;
              catCur = pageBase + ((ni < pageN) ? ni : (row * HUB_COLS < pageN ? row * HUB_COLS : pageN - 1)); redraw = 1; }
            if (down & (BUTTON_LEFT | BUTTON_RIGHT))
            { int nc = (1 - col) + row * HUB_COLS; if (nc < pageN) catCur = pageBase + nc; redraw = 1; }
            if ((down & BUTTON_L1) && hubPages > 1)
            { int np = (page - 1 + hubPages) % hubPages, npBase = np * HUB_PAGESZ, npN = CHK_NCATS - npBase; if (npN > HUB_PAGESZ) npN = HUB_PAGESZ;
              catCur = npBase + (local < npN ? local : npN - 1); redraw = 1; }
            if ((down & BUTTON_R1) && hubPages > 1)
            { int np = (page + 1) % hubPages, npBase = np * HUB_PAGESZ, npN = CHK_NCATS - npBase; if (npN > HUB_PAGESZ) npN = HUB_PAGESZ;
              catCur = npBase + (local < npN ? local : npN - 1); redraw = 1; }
            if (tap)
            {
                for (int li = 0; li < pageN; ++li)
                {
                    int c2 = li % HUB_COLS, r2 = li / HUB_COLS, bx = CHKB_L + c2 * (CHKB_COLW + 8), by = CHKB_TOP + r2 * CHKB_STRIDE;
                    if (px >= bx && px < bx + CHKB_COLW && py >= by && py < by + CHKB_BTN_H)
                    { catCur = pageBase + li; level = 1; catCursor[pageBase + li] = 0; catScroll[pageBase + li] = 0; lastCur = -1000; redraw = 1; }
                }
                int gridBottom = CHKB_TOP + HUB_ROWS * CHKB_STRIDE - (CHKB_STRIDE - CHKB_BTN_H);
                int fy = gridBottom + 20;
                if (px >= CHKB_L && px < CHKB_R && py >= fy && py < fy + 24)
                { ChecklistAutoFill(); ChecklistSave(); ChkAfMsg(afBuf, &afMsg);
                  afTick = 180; redraw = 1; }
            }
            if (down & BUTTON_Y) { ChecklistAutoFill(); ChecklistSave(); ChkAfMsg(afBuf, &afMsg);
                afTick = 180; redraw = 1; }
            if (down & BUTTON_START) { ChecklistReset(); afMsg = "Reset"; afTick = 180; lastCur = -1000; redraw = 1; }
            if (down & BUTTON_A) { level = 1; catCursor[catCur] = 0; catScroll[catCur] = 0; lastCur = -1000; redraw = 1; }
            if (afTick > 0 && --afTick == 0) { afMsg = NULL; redraw = 1; } // clear the button result on timeout
        }
        else
        {
            const ChkCat *cc = &CHK_CATS[catCur];
            int cursor = catCursor[catCur], scroll = catScroll[catCur];

            // filtered index list for this category
            int filtIdx[CHK_MAXITEMS], filtN = 0;
            for (int i = 0; i < cc->count; ++i)
            {
                int checked = (chkState[catCur][i] == 1 || chkState[catCur][i] == 2);
                if (filterMode == 1 && checked) continue;
                if (filterMode == 2 && !checked) continue;
                filtIdx[filtN++] = i;
            }
            if (cursor >= filtN) cursor = filtN > 0 ? filtN - 1 : 0;

            if (down & BUTTON_B) { level = 0; redraw = 1; }
            if (down & BUTTON_L1) { catCur = (catCur + CHK_NCATS - 1) % CHK_NCATS; lastCur = -1000; redraw = 1; }
            if (down & BUTTON_R1) { catCur = (catCur + 1) % CHK_NCATS; lastCur = -1000; redraw = 1; }
            if (down & BUTTON_Y)  { filterMode = (filterMode + 1) % 3; redraw = 1; }
            if (down & BUTTON_X)  { revealLoc = !revealLoc; redraw = 1; }
            if (down & BUTTON_DOWN && filtN > 0) { cursor = (cursor + 1) % filtN; redraw = 1; }
            if (down & BUTTON_UP   && filtN > 0) { cursor = (cursor - 1 + filtN) % filtN; redraw = 1; }
            // D-Pad Left/Right page through the item list (8 rows visible); shoulders stay area-switch
            if (down & BUTTON_RIGHT && filtN > 0) { cursor += 8; if (cursor >= filtN) cursor = filtN - 1; redraw = 1; }
            if (down & BUTTON_LEFT  && filtN > 0) { cursor -= 8; if (cursor < 0) cursor = 0; redraw = 1; }

            if (tap && filtN > 0)
            {
                for (int r = 0; r < 8 && scroll + r < filtN; ++r)
                    if (px >= CHKB_L && px < CHKB_R && py >= 34 + r * 18 && py < 34 + r * 18 + 18)
                    { cursor = scroll + r; redraw = 1; }
            }
            if ((down & BUTTON_A) && filtN > 0)
            {
                int gi = filtIdx[cursor];
                int checked = (chkState[catCur][gi] == 1 || chkState[catCur][gi] == 2);
                chkState[catCur][gi] = (u8)(checked ? 3 : 2);
                ChecklistSave(); redraw = 1;
            }

            catCursor[catCur] = cursor; catScroll[catCur] = scroll;
            if (cursor != lastCur) { selTick = 0; lastCur = cursor; }
            else if (selTick < 100000) selTick++;

            // Mirrors CText6Marquee's own "tw > w && tick >= delay" trigger for each of the 3
            // marqueed fields on the current item, WITHOUT drawing anything - lets the redraw
            // gate below stay cheap (skip drawing) until a marquee is genuinely about to scroll,
            // instead of flipping to "redraw every tick" as soon as any delay elapses regardless
            // of whether that field even needs to scroll. Getting this wrong is exactly what
            // made the hint's 2s delay measure as ~10s before: once selTick passed the list-row
            // delay (62 ticks, ~1s), every tick started paying the ~100ms+ full-redraw cost even
            // though the hint field's own 124-tick delay hadn't elapsed yet - so the hint delay
            // was actually being counted in ~100ms+ ticks instead of ~16ms ones.
            if (filtN > 0)
            {
                const ChkItem *sit = &cc->items[filtIdx[cursor]];
                if (selTick >= CHK_MARQUEE_DELAY      && C6Width(sit->task) > CHK_TASK_TXTW) selOverflow = 1;
                if (selTick >= CHK_HINT_MARQUEE_DELAY && C6Width(sit->hint) > CHK_HINT_SVW)   selOverflow = 1;
                if (revealLoc && sit->loc[0] && selTick >= CHK_MARQUEE_DELAY && C6Width(sit->loc) > CHK_HINT_SVW - 6) selOverflow = 1;
            }
        }

        // A full redraw here means re-compositing the whole backdrop + bottom-screen frame -
        // expensive on this hardware (~100ms+). Only pay that cost when something actually
        // changed (redraw==1) or a marquee is genuinely mid-scroll (tick past its delay). While
        // just WAITING for a delay to elapse, nothing on screen changes, so we skip the redraw
        // entirely and let the ~16ms sleep above be the only per-tick cost - that's what keeps
        // the 1s/2s delays accurate; redrawing every tick during the wait was inflating each
        // "tick" to ~127ms, which is exactly why the delay measured ~8x too long.
        int animating = (level == 1) && selOverflow; // hub has no marquee anymore
        if (!redraw && !animating) continue;
        redraw = 0;

        // =========================================================== DRAW
        if (level == 0)
        {
            int page = catCur / HUB_PAGESZ, pageBase = page * HUB_PAGESZ;
            int pageN = CHK_NCATS - pageBase; if (pageN > HUB_PAGESZ) pageN = HUB_PAGESZ;

            ComposeBackdrop();
            CText(WIN_X + 12, WIN_Y + 7, T("100% Checklist"), INK, 1);
            CFill(WIN_X + 12, WIN_Y + 24, CTextWidth(T("100% Checklist")) + 6, 1, GOLD);
            if (hubPages > 1)
            {
                char pg[16]; siprintf(pg, "%d/%d", page + 1, hubPages);
                CText6(WIN_X + WIN_W - 12 - C6Width(pg), WIN_Y + 9, pg, INK_DIM);
            }
            int totalDone = 0, totalAll = 0;
            for (int c = 0; c < CHK_NCATS; ++c) // totals always cover ALL categories, not just this page
            {
                int done = 0;
                for (int i = 0; i < CHK_CATS[c].count; ++i)
                    if (chkState[c][i] == 1 || chkState[c][i] == 2) ++done;
                totalDone += done; totalAll += CHK_CATS[c].count;
            }
            int colW = ROW_W / 2, gridY = WIN_Y + 30, rowH = 28, rowGap = 3;
            for (int li = 0; li < pageN; ++li)
            {
                int c = pageBase + li;
                int done = 0;
                for (int i = 0; i < CHK_CATS[c].count; ++i)
                    if (chkState[c][i] == 1 || chkState[c][i] == 2) ++done;
                int col = li % HUB_COLS, row = li / HUB_COLS;
                int x = ROW_X + col * colW, y = gridY + row * (rowH + rowGap);
                int full = (CHK_CATS[c].count > 0 && done == CHK_CATS[c].count);
                int selc = (c == catCur);
                const u8 *nc = full ? CGREEN : (selc ? CGOLD : CINK);
                if (selc) CFillBlend(x - 3, y, colW - 6, rowH, 0, 0, 0, 110);
                char frac[16]; siprintf(frac, "%d/%d", done, CHK_CATS[c].count);
                int fw = C6Width(frac);
                int nameW = colW - fw - 16;
                CTextWrap2(x, y, nameW, rowH, CHK_CATS[c].name, nc[0], nc[1], nc[2]);
                CText6(x + colW - 12 - fw, y + (rowH - FONT_HEIGHT) / 2, frac, nc[0], nc[1], nc[2]);
            }
            int ty = gridY + HUB_ROWS * (rowH + rowGap) + 4;
            CFill(WIN_X + 12, ty, WIN_W - 24, 1, GOLD);
            CText6(WIN_X + 12, ty + 8, "Total", INK);
            char totFrac[24]; int pct = totalAll > 0 ? totalDone * 100 / totalAll : 0;
            siprintf(totFrac, "%d/%d (%d%%)", totalDone, totalAll, pct);
            CText6(WIN_X + WIN_W - 12 - C6Width(totFrac), ty + 8, totFrac, GREEN_ON);
            CFillInset(WIN_X + 12, ty + 20, WIN_W - 24, 5, 1);
            int barw = totalAll > 0 ? (WIN_W - 24) * totalDone / totalAll : 0;
            CFill(WIN_X + 12, ty + 20, barw, 5, GREEN_ON);
            int lx = WIN_X + 12, ly = ty + 31;
            const char *chips[4] = { "auto", "you", "todo", "cleared" };
            const u8 *chipc[4] = { CGREEN, CGOLD, CDIM, CDIM };
            for (int i = 0; i < 4; ++i)
            {
                CFill(lx, ly, 8, 8, chipc[i][0], chipc[i][1], chipc[i][2]);
                CText6(lx + 12, ly, chips[i], INK_DIM);
                lx += 12 + C6Width(chips[i]) + 6;
            }
            Present(); Present();

            for (int y = 0; y < BOT_H; ++y)
                for (int x = 0; x < BOT_W; ++x)
                {
                    u8 *p = CPix(x, y);
                    if (savedBotValid)
                    { u16 v = savedBot[y * BOT_W + x];
                      p[0] = (u8)(((v>>11)&31)<<3); p[1] = (u8)(((v>>5)&63)<<2); p[2] = (u8)((v&31)<<3); }
                    else { p[0] = p[1] = p[2] = 12; }
                }
            CFillBlend(0, 0, BOT_W, BOT_H, BG, 230);
            CFill(6, 4, BOT_W - 12, 1, GOLD); CFill(6, BOT_H - 6, BOT_W - 12, 1, GOLD);
            for (int li = 0; li < pageN; ++li)
            {
                int i = pageBase + li;
                int col = li % HUB_COLS, row = li / HUB_COLS, bx = CHKB_L + col * (CHKB_COLW + 8), by = CHKB_TOP + row * CHKB_STRIDE;
                int done = 0;
                for (int k = 0; k < CHK_CATS[i].count; ++k)
                    if (chkState[i][k] == 1 || chkState[i][k] == 2) ++done;
                int full = (done == CHK_CATS[i].count);
                int sel = (i == catCur);
                const u8 *bc = full ? CGREEN : CGOLD;
                CFill(bx, by, CHKB_COLW, CHKB_BTN_H, sel ? 52 : 32, sel ? 44 : 25, sel ? 26 : 16);
                CFill(bx, by, CHKB_COLW, 1, bc[0], bc[1], bc[2]); CFill(bx, by + CHKB_BTN_H - 1, CHKB_COLW, 1, bc[0], bc[1], bc[2]);
                CFill(bx, by, 1, CHKB_BTN_H, bc[0], bc[1], bc[2]); CFill(bx + CHKB_COLW - 1, by, 1, CHKB_BTN_H, bc[0], bc[1], bc[2]);
                if (sel) CFill(bx, by, 4, CHKB_BTN_H, 255, 255, 255); // bright left bar - unmistakably "selected"
                // No fraction here - the top screen already shows X/Y per category, repeating it
                // on the touch buttons just stole width from the name and forced truncation.
                CTextWrap2CenterBox(bx + 6, by, CHKB_COLW - 12, CHKB_BTN_H, CHK_CATS[i].name, bc[0], bc[1], bc[2]);
            }
            int gridBottom = CHKB_TOP + HUB_ROWS * CHKB_STRIDE - (CHKB_STRIDE - CHKB_BTN_H);
            // above the Auto-fill box: navigation hints, horizontally centered
            const char *hUp = hubPages > 1 ? T("{DP} move   {A} open   {L}/{R} areas") : T("{DP} move   {A} open");
            CText6Btn((BOT_W - C6BtnWidth(hUp)) / 2, gridBottom + 6, hUp, INK_DIM);
            int fy = gridBottom + 20;
            CFill(CHKB_L, fy, CHKB_R - CHKB_L, 24, 20, 40, 20);
            CFill(CHKB_L, fy, CHKB_R - CHKB_L, 1, GREEN_ON); CFill(CHKB_L, fy + 23, CHKB_R - CHKB_L, 1, GREEN_ON);
            CFill(CHKB_L, fy, 1, 24, GREEN_ON); CFill(CHKB_R - 1, fy, 1, 24, GREEN_ON);
            int atw = C6Width(T("Auto-fill from save"));
            CText6(CHKB_L + (CHKB_R - CHKB_L - atw) / 2, fy + 7, T("Auto-fill from save"), GREEN_ON);
            if (afMsg) // on-screen result, right side of the button (no need to leave the screen)
            {
                const char *m = T(afMsg);
                if (afMsg[0] == 'N' || afMsg[0] == 'R') CText6(CHKB_R - 8 - C6Width(m), fy + 7, m, INK_DIM); // Nothing new / Reset
                else                                    CText6(CHKB_R - 8 - C6Width(m), fy + 7, m, 255, 236, 120); // OK: +N
            }
            // below the box: action hints, horizontally centered (START has no glyph, shown as text)
            const char *hDn = T("{Y} Auto-fill    START reset");
            CText6Btn((BOT_W - C6BtnWidth(hDn)) / 2, fy + 30, hDn, INK_DIM);
            BotBlitComposeBoth();
        }
        else
        {
            const ChkCat *cc = &CHK_CATS[catCur];
            int cursor = catCursor[catCur], scroll = catScroll[catCur];
            int filtIdx[CHK_MAXITEMS], filtN = 0;
            for (int i = 0; i < cc->count; ++i)
            {
                int checked = (chkState[catCur][i] == 1 || chkState[catCur][i] == 2);
                if (filterMode == 1 && checked) continue;
                if (filterMode == 2 && !checked) continue;
                filtIdx[filtN++] = i;
            }
            if (cursor >= filtN) cursor = filtN > 0 ? filtN - 1 : 0;
            if (cursor < scroll) scroll = cursor;
            if (cursor >= scroll + 8) scroll = cursor - 7;
            catCursor[catCur] = cursor; catScroll[catCur] = scroll;

            ComposeBackdrop();
            CTextClip(WIN_X + 12, WIN_Y + 7, cc->name, 200, INK, 1);
            int done = 0; for (int i = 0; i < cc->count; ++i) if (chkState[catCur][i]==1||chkState[catCur][i]==2) ++done;
            char hdr[16]; siprintf(hdr, "%d/%d", done, cc->count);
            CText6(WIN_X + WIN_W - 12 - C6Width(hdr), WIN_Y + 9, hdr, INK_DIM);
            CFill(WIN_X + 12, WIN_Y + 22, WIN_W - 24, 1, GOLD);

            if (filtN > 0)
            {
                int gi = filtIdx[cursor];
                const ChkItem *it = &cc->items[gi];
                int bx = WIN_X + 12, by = WIN_Y + 34;
                CFillInset(bx, by, 66, 66, 0); CFill(bx, by, 66, 1, GOLD); CFill(bx, by+65, 66, 1, GOLD);
                CFill(bx, by, 1, 66, GOLD); CFill(bx+65, by, 1, 66, GOLD);
                DrawChkIcon(it, bx + 13, by + 13, 5); // 40px icon, centered in the 66px box
                int tx = bx + 76;
                CTextClip(tx, by, it->task, WIN_X + WIN_W - 12 - tx, GOLD, 0);
                u8 st = chkState[catCur][gi];
                int fy2 = by + 72;
                CFill(WIN_X + 12, fy2, WIN_W - 24, 1, 120, 98, 50); // hairline
                int ly2 = fy2 + 10;
                CText6(WIN_X + 12, ly2, T("Status"), GOLD);
                const char *statTxt = st == 1 ? T("From your save") : st == 2 ? T("Checked by you")
                                     : st == 3 ? T("You cleared this") : T("Not done yet");
                const u8 *statC = st == 1 ? CGREEN : st == 2 ? CGOLD : st == 3 ? CGOLD : CDIM;
                u8 sc[3]; LiftForDark(statC[0], statC[1], statC[2], sc); // keep legible on the dark pill (dark-accent themes)
                int svx = WIN_X + 86, svw = WIN_X + WIN_W - 12 - svx;
                CFillInset(svx, ly2 - 2, svw, 14, 1);
                CFill(svx, ly2 - 2, svw, 1, sc[0], sc[1], sc[2]); CFill(svx, ly2+11, svw, 1, sc[0], sc[1], sc[2]);
                CText6Clip(svx + (svw - C6Width(statTxt)) / 2, ly2, statTxt, svw - 6, sc[0], sc[1], sc[2]);

                int hy = ly2 + 24;
                CText6(WIN_X + 12, hy, T("Hint"), GOLD);
                CText6Marquee(svx, hy, svw, it->hint, selTick, CHK_HINT_MARQUEE_DELAY, CHK_SPEED_FAST, INK);

                int oy = hy + 20;
                CText6(WIN_X + 12, oy, T("Where"), GOLD);
                if (!it->loc[0]) CText6(svx, oy, "-", INK_DIM);
                else if (!revealLoc)
                {
                    CFillInset(svx, oy - 2, svw, 14, 1);
                    CFill(svx, oy - 2, svw, 1, GREEN_ON); CFill(svx, oy+11, svw, 1, GREEN_ON);
                    const char *lk = T("X: show location");
                    CText6Clip(svx + (svw - C6Width(lk)) / 2, oy, lk, svw - 6, GREEN_ON);
                }
                else
                {
                    CFillInset(svx, oy - 2, svw, 14, 1);
                    CFill(svx, oy - 2, svw, 1, GREEN_ON); CFill(svx, oy+11, svw, 1, GREEN_ON);
                    CText6Marquee(svx + 3, oy, svw - 6, it->loc, selTick, CHK_MARQUEE_DELAY, CHK_SPEED_FAST, GREEN_ON);
                }
            }
            else CText6Clip(WIN_X + 12, WIN_Y + 100, T("Nothing here - try Y to change the filter."), WIN_W - 24, INK_DIM);
            CText6Btn(WIN_X + 12, WIN_Y + WIN_H - 14, T("{L}/{R} area   {X} location"), INK_DIM);
            Present(); Present();

            for (int y = 0; y < BOT_H; ++y)
                for (int x = 0; x < BOT_W; ++x)
                {
                    u8 *p = CPix(x, y);
                    if (savedBotValid)
                    { u16 v = savedBot[y * BOT_W + x];
                      p[0] = (u8)(((v>>11)&31)<<3); p[1] = (u8)(((v>>5)&63)<<2); p[2] = (u8)((v&31)<<3); }
                    else { p[0] = p[1] = p[2] = 12; }
                }
            CFillBlend(0, 0, BOT_W, BOT_H, BG, 230);
            CFill(6, 4, BOT_W - 12, 1, GOLD); CFill(6, BOT_H - 6, BOT_W - 12, 1, GOLD);
            const char *fl = filterMode == 0 ? "All" : filterMode == 1 ? "Todo" : "Done";
            char flbuf[24]; sniprintf(flbuf, sizeof flbuf, "%s %d/%d", fl, done, cc->count);
            CText6(CHKB_L, 8, "<", INK_DIM);
            CTextClip(20, 6, cc->name, 190, GOLD, 0);
            CText6(CHKB_R - 8 - C6Width(flbuf) - 10, 8, flbuf, INK_DIM);
            CText6(CHKB_R - 8, 8, ">", INK_DIM);
            CFill(CHKB_L, 26, CHKB_R - CHKB_L, 1, GOLD);
            for (int r = 0; r < 8 && scroll + r < filtN; ++r)
            {
                int gi = filtIdx[scroll + r]; int y = 34 + r * 18;
                int isCur = (scroll + r == cursor);
                if (isCur) CFillBlend(CHKB_L, y - 1, CHKB_R - CHKB_L, 18, 0, 0, 0, 110);
                const ChkItem *it = &cc->items[gi];
                DrawChkIcon(it, CHKB_L, y + 1, 2);
                u8 st = chkState[catCur][gi];
                int tx = CHKB_L + 16 + 8; // icon is 16px wide - 8px gap so text doesn't look glued to it
                int sx = CHKB_R - 16, sy = y + 3, txtw = sx - 6 - tx;
                if (isCur) CText6Marquee(tx, y + 4, txtw, it->task, selTick, CHK_MARQUEE_DELAY, CHK_SPEED_LIST, 236, 200, 120);
                else       CText6Clip(tx, y + 4, it->task, txtw, 236, 236, 210);
                if (st == 1) CFill(sx, sy, 9, 9, GREEN_ON);
                else if (st == 2) CFill(sx, sy, 9, 9, GOLD);
                else { CFill(sx, sy, 9, 1, 140,130,104); CFill(sx, sy+8, 9, 1, 140,130,104);
                       CFill(sx, sy, 1, 9, st==3?230:140, st==3?200:130, st==3?90:104);
                       CFill(sx+8, sy, 1, 9, st==3?230:140, st==3?200:130, st==3?90:104); }
            }
            if (filtN == 0) CText6(140, 100, T("Nothing here."), INK_DIM);
            // scroll arrows (same gold triangles the main menu uses): more items above/below
            int sax = (CHKB_L + CHKB_R) / 2;
            if (scroll > 0)
                for (int i = 0; i < 4; ++i) { int w = 1 + 2 * i; CFill(sax - w / 2, 28 + i, w, 1, GOLD); }
            if (scroll + 8 < filtN)
                for (int i = 0; i < 4; ++i) { int w = 7 - 2 * i; CFill(sax - w / 2, 190 + i, w, 1, GOLD); }
            { const char *hf = T("{DP} move   {A} mark/clear   {Y} filter");
              CText6Btn((BOT_W - C6BtnWidth(hf)) / 2, 210, hf, INK_DIM); }
            BotBlitComposeBoth();
        }
    }
}

#endif // !TOOLS_ONLY

static void ToolRun(int t)
{
    if (t == T_SEARCH) ToolSearch();
    else if (t == T_RAMDUMP) ToolRamDump();
    else if (t == T_HEXEDIT) ToolHexEdit();
    else if (t == T_ABOUT) ToolAbout();
#if !TOOLS_ONLY
    else if (t == T_GAMEGUIDE) ToolGameGuide();
    else if (t == T_TRACKER)   ToolChecklist();
#endif
    else if (t == T_PLUGINGUIDE) ToolPluginGuide();
}

// ===================== Game pause (Luma thread scheduler) =====================
#define THREADVARS_MAGIC  0x21545624
static bool ThreadPredicate(void *thread_)
{
    u32   tls     = *(volatile u32 *)((u8 *)thread_ + 0x94);
    void *current = *(void **)0xFFFF9000;
    if (current != thread_ && *(volatile u32 *)tls != THREADVARS_MAGIC) return true;
    return false;
}
static void PauseGame(void)  { svcControlProcess(CUR_PROCESS_HANDLE, PROCESSOP_SCHEDULE_THREADS, 1, (u32)ThreadPredicate); }
static void ResumeGame(void) { svcControlProcess(CUR_PROCESS_HANDLE, PROCESSOP_SCHEDULE_THREADS, 0, (u32)ThreadPredicate); }

// ===================== Menu loop =====================
// Navigation state is persistent: reopening the menu returns to the last spot.
static int fstack[8], cstack[8], sstack[8], menuDepth = 0;
static int menuFolder = F_ROOT, menuCursor = 1, menuScroll = 0; // cursor 1 = first HOME item (0 is a separator)

// Is a theme's background light? (same luminance heuristic as ThemeBgLight, for any theme)
static int ThemeIdxLight(int i)
{ const Theme *t = &THEMES[i]; return (t->bg[0]*30 + t->bg[1]*59 + t->bg[2]*11)/100 > 140; }

// Build the list of theme indices matching a filter (0=all, 1=light, 2=dark). Returns the count.
static int ThemeFilterBuild(int filt, int *flt)
{
    int fn = 0;
    for (int i = 0; i < THEME_COUNT; ++i)
    {
        int light = ThemeIdxLight(i);
        if (filt == 1 && !light) continue;
        if (filt == 2 && light) continue;
        flt[fn++] = i;
    }
    return fn;
}

// Live theme picker (Settings -> Change Theme). Moving the cursor previews the
// theme instantly (the whole list recolors). A keeps it (saved on menu close),
// B reverts to the theme active on entry, SELECT keeps it and jumps to the game.
// Y cycles a light/dark filter so long lists are quicker to sort through.
static void ThemePicker(void)
{
    static int themeFilt = 0;           // 0=all 1=light 2=dark, persists across opens
    int startIdx = g_themeIdx;
    int flt[THEME_COUNT];
    int fn = ThemeFilterBuild(themeFilt, flt);
    if (fn == 0) { themeFilt = 0; fn = ThemeFilterBuild(0, flt); }
    int sel = 0;                         // index INTO flt[]
    for (int i = 0; i < fn; ++i) if (flt[i] == g_themeIdx) { sel = i; break; }
    int scroll = 0, redraw = 1;
    u32 prev = HID_PAD;
    while (1)
    {
        if (sel < scroll) scroll = sel;
        if (sel >= scroll + MAX_ROWS) scroll = sel - MAX_ROWS + 1;
        if (redraw)
        {
            ApplyTheme(flt[sel]); // live preview: recolors everything drawn below
            ComposeBackdrop();
            CText(WIN_X + 12, WIN_Y + 7, T("Change Theme"), INK, 1);
            CFill(WIN_X + 12, WIN_Y + 24, CTextWidth("Change Theme") + 6, 1, GOLD);
            char pos[16]; siprintf(pos, "%d/%d", sel + 1, fn);
            CText6(WIN_X + WIN_W - 12 - C6Width(pos), WIN_Y + 9, pos, INK_DIM);
            for (int r = 0; r < MAX_ROWS; ++r)
            {
                int fi = scroll + r; if (fi >= fn) break;
                int i = flt[fi];
                int y = ROW_Y0 + r * ROW_H;
                if (fi == sel)
                {
                    CFillBlend(ROW_X - 4, y - 1, ROW_W + 8, ROW_H, 0, 0, 0, 110);
                    CFill(ROW_X - 4, y - 1, 2, ROW_H, GOLD);
                }
                const Theme *t = &THEMES[i];
                int sx = ROW_X + 2; // 4 color swatches: bg / gold / text / on
                CFill(sx,      y + 3, 8, 9, t->bg[0], t->bg[1], t->bg[2]);
                CFill(sx + 9,  y + 3, 8, 9, t->gold[0], t->gold[1], t->gold[2]);
                CFill(sx + 18, y + 3, 8, 9, t->ink[0], t->ink[1], t->ink[2]);
                CFill(sx + 27, y + 3, 8, 9, t->green[0], t->green[1], t->green[2]);
                const u8 *tc = (fi == sel) ? CGREEN : CINK;
                CText(sx + 40, y - 1, t->name, tc[0], tc[1], tc[2], 0);
            }
            if (scroll > 0)
                for (int a = 0; a < 4; ++a) CFill(WIN_X + WIN_W - 14 - a, ROW_Y0 + 3 + a, 1 + 2 * a, 1, GOLD);
            if (scroll + MAX_ROWS < fn)
                for (int a = 0; a < 4; ++a) CFill(WIN_X + WIN_W - 14 - a, ROW_Y0 + MAX_ROWS * ROW_H - 4 - a, 1 + 2 * a, 1, GOLD);
            const char *fmode = themeFilt == 1 ? T("{Y} light") : themeFilt == 2 ? T("{Y} dark") : T("{Y} all");
            char leg[96]; sniprintf(leg, sizeof leg, "%s  %s  %s  %s", T("{A} apply"), T("{B} cancel"), T("{L}/{R} page"), fmode);
            CText6Btn(WIN_X + 12, WIN_Y + WIN_H - 16, leg, INK_DIM);
            Present(); Present();
            ComposeBottom(); BotBlitComposeBoth(); // live-recolor the bottom screen with the previewed theme
            redraw = 0;
        }
        svcSleepThread(16 * 1000 * 1000);
        u32 pad = HID_PAD, down = ARepeat(pad, &prev, &g_arHold);
        if (down & BUTTON_DOWN) { sel = (sel + 1 < fn) ? sel + 1 : 0; redraw = 1; }
        if (down & BUTTON_UP)   { sel = (sel > 0) ? sel - 1 : fn - 1; redraw = 1; }
        if (down & (BUTTON_RIGHT | BUTTON_R1)) { sel += MAX_ROWS; if (sel >= fn) sel = fn - 1; redraw = 1; }
        if (down & (BUTTON_LEFT | BUTTON_L1))  { sel -= MAX_ROWS; if (sel < 0) sel = 0; redraw = 1; }
        if (down & BUTTON_Y)    { int cur = flt[sel]; // keep the current theme selected across the filter change
                                  themeFilt = (themeFilt + 1) % 3;
                                  fn = ThemeFilterBuild(themeFilt, flt);
                                  if (fn == 0) { themeFilt = 0; fn = ThemeFilterBuild(0, flt); }
                                  sel = 0; for (int i = 0; i < fn; ++i) if (flt[i] == cur) { sel = i; break; }
                                  scroll = 0; redraw = 1; }
        if (down & BUTTON_A)      { ApplyTheme(flt[sel]); configDirty = 1; QueueToastRaw("Theme:", THEMES[flt[sel]].name); return; }
        if (down & BUTTON_B)      { ApplyTheme(startIdx); ComposeBottom(); BotBlitComposeBoth(); return; } // revert the previewed bottom too
        if (down & BUTTON_SELECT) { ApplyTheme(flt[sel]); configDirty = 1; g_quitToGame = 1; return; }
    }
}

// First-launch language chooser. Live-previews each language as you scroll;
// any of A/B/SELECT confirms (English stays the default if untouched).
static void LanguagePicker(void)
{
    int sel = g_langIdx;
    u32 prev = HID_PAD;
    int redraw = 1;
    LangProbeAvail(); // refresh which languages have SD files, so missing ones show red
    while (1)
    {
        if (redraw)
        {
            g_langIdx = sel; LangLoad(); // live preview
            ComposeBackdrop();
            CText(WIN_X + 12, WIN_Y + 7, T("Select Language"), INK, 1);
            CFill(WIN_X + 12, WIN_Y + 24, CTextWidth(T("Select Language")) + 6, 1, GOLD);
            for (int i = 0; i < NUM_LANGS; ++i)
            {
                int y = ROW_Y0 + i * ROW_H;
                if (i == sel)
                {
                    CFillBlend(ROW_X - 4, y - 1, ROW_W + 8, ROW_H, 0, 0, 0, 110);
                    CFill(ROW_X - 4, y - 1, 2, ROW_H, GOLD);
                }
                if (!g_langAvail[i])   CText(ROW_X + 10, y - 1, kLangLabels[i], 225, 60, 45, 0);   // no SD file -> red
                else if (i == sel)     CText(ROW_X + 10, y - 1, kLangLabels[i], GREEN_ON, 0);
                else                   CText(ROW_X + 10, y - 1, kLangLabels[i], INK, 0);
            }
            CText6(WIN_X + 12, WIN_Y + WIN_H - 16, T("A: confirm"), INK_DIM);
            Present(); Present();
            redraw = 0;
        }
        svcSleepThread(16 * 1000 * 1000);
        u32 pad = HID_PAD, down = ARepeat(pad, &prev, &g_arHold);
        if (down & BUTTON_DOWN) { sel = (sel + 1 < NUM_LANGS) ? sel + 1 : 0; redraw = 1; }
        if (down & BUTTON_UP)   { sel = (sel > 0) ? sel - 1 : NUM_LANGS - 1; redraw = 1; }
        if (down & (BUTTON_A | BUTTON_SELECT)) // any A/SELECT confirms; there's no cancel on first run
        {
            g_langIdx = sel; LangLoad(); GuideLoad(); configDirty = 1; // save so it won't ask again
            return;
        }
    }
}

// Cycle a Settings "picker" cheat by dir (+1 next / -1 previous). Shared by the A button (dir=+1) and
// D-pad ←/→, so both stay in sync. Applies the same side effects/toasts as the old A-only handlers.
// Returns 1 if it handled `cheat`, 0 otherwise (e.g. Theme, which opens a full picker instead).
static int CfgCycle(int cheat, int dir)
{
    int step = (dir > 0) ? 1 : -1;
    switch (cheat)
    {
        case CH_CFG_QMKEY:
            qmCombo = (qmCombo + step + NUM_QMCOMBOS) % NUM_QMCOMBOS;
            QueueToastRaw(qmCombos[qmCombo].plain, ": SET"); configDirty = 1; return 1;
        case CH_CFG_HK1:
            hk1 = (hk1 + step + NUM_HOTKEYS) % NUM_HOTKEYS;
            QueueToastRaw("Time Scrub: ", hotKeys[hk1].glyph); configDirty = 1; return 1;
        case CH_CFG_HK2:
            hk2 = (hk2 + step + NUM_HOTKEYS) % NUM_HOTKEYS;
            QueueToastRaw("Day Scrub: ", hotKeys[hk2].glyph); configDirty = 1; return 1;
        case CH_CFG_LANG:
            g_langIdx = (g_langIdx + step + NUM_LANGS) % NUM_LANGS;
            LangLoad(); GuideLoad(); QueueToastRaw(kLangLabels[g_langIdx], "");
            ComposeBottom(); BotBlitComposeBoth(); // relocalize the bottom legend now
            configDirty = 1; return 1;
    }
    return 0;
}

static void RunMenu(void)
{
    int depth = menuDepth;
    int folderIdx = menuFolder, cursor = menuCursor, scroll = menuScroll;

    if (!gCompose) return;
    SysFontInit();
    g_quitToGame = 0; // fresh; a sub-loop sets this to request "exit to game"

    PauseGame();
    TopTakeOver();   // remember which buffer the game was showing, so we can hand it back

    BotGrab();
    ComposeBottom();
    BotBlitComposeBoth();

    if (g_qmHandoff)
    {
        g_qmHandoff = 0;
        RestoreTopBackdrop();  // savedTop = the real game frame, saved before the panel was drawn
    }
    else
    {
        GrabFb();
    }
    DimOutsideWindow();
    CaptureTopBackdrop(); // save clean backdrop so top redraws stay bleed-free

    // Draw the top-screen menu NOW, before waiting for SELECT to be released - otherwise the top
    // stays blank (game backdrop only) until the user lets go of SELECT, while the bottom is
    // already up. Skip it for the first-run language picker / tool-resume paths, which paint their
    // own screens right after the wait (an early menu frame would just flash before them).
    if (!g_firstRun && g_resumeTool < 0)
    {
        ComposeMenu(&folders[folderIdx], depth, cursor, scroll);
        Present(); Present();
    }

    DrainButtons(BUTTON_SELECT);   // capped: a stuck pad must not hang the console with the game paused
    u32 prev = HID_PAD;

    // First ever launch (no Settings.cfg): ask for a language before anything else.
    if (g_firstRun && fsReady)
    {
        g_firstRun = 0;
        LanguagePicker();
        ComposeBottom(); BotBlitComposeBoth(); // bottom legend in the chosen language
        prev = HID_PAD;
    }

    // Coming back from an exit-to-game that happened inside a tool: drop the
    // player straight back into that tool instead of the Tools folder.
    if (g_resumeTool >= 0)
    {
        int t = g_resumeTool; g_resumeTool = -1;
        ToolRun(t);
        if (g_quitToGame) g_resumeTool = t;                  // SELECT again -> keep resuming
        else { ComposeBottom(); BotBlitComposeBoth(); prev = HID_PAD; }
    }

    if (!g_quitToGame)
    {
    ComposeMenu(&folders[folderIdx], depth, cursor, scroll);
    Present();
    Present();

    while (1)
    {
        svcSleepThread(16 * 1000 * 1000);
        u32 pad = HID_PAD, down = ARepeat(pad, &prev, &g_arHold);

        const Folder *fld = &folders[folderIdx];
        int changed = 0;

        if (folderIdx == F_ROOT || (folderIdx == F_TELEPORT && g_tpFilter == 0)) // grouped 2-column grid
        {
            BuildRootLayout(fld);
            if (NavSkip(folderIdx, cursor)) cursor = RootFirstSel(fld);
            if (down & BUTTON_UP)    { cursor = RootNeighbor(fld, cursor, 0); changed = 1; }
            if (down & BUTTON_DOWN)  { cursor = RootNeighbor(fld, cursor, 1); changed = 1; }
            if (down & BUTTON_LEFT)  { cursor = RootNeighbor(fld, cursor, 2); changed = 1; }
            if (down & BUTTON_RIGHT) { cursor = RootNeighbor(fld, cursor, 3); changed = 1; }
        }
        else
        {
            // never rest on a non-selectable row (section header OR a filtered-out Teleport item)
            if (NavSkip(folderIdx, cursor)) { int c = cursor; do { c = (c + 1 < fld->count) ? c + 1 : 0; } while (NavSkip(folderIdx, c) && c != cursor); if (c != cursor) changed = 1; cursor = c; }
            // wrap-around navigation, skipping headers and hidden rows
            if (down & BUTTON_DOWN) { int c = cursor; do { c = (c + 1 < fld->count) ? c + 1 : 0; } while (NavSkip(folderIdx, c) && c != cursor); cursor = c; changed = 1; }
            if (down & BUTTON_UP)   { int c = cursor; do { c = (c > 0) ? c - 1 : fld->count - 1; } while (NavSkip(folderIdx, c) && c != cursor); cursor = c; changed = 1; }
            // On a "cycler" row (Language, the hotkey binders), D-pad left/right change the value
            // and the shoulders L/R still page. Everywhere else left/right and the shoulders all
            // jump a page, clamped to the ends, skipping headers and hidden rows.
            int rowCheat = fld->items[cursor].cheat;
            int isCycler = (rowCheat == CH_CFG_LANG || rowCheat == CH_CFG_QMKEY ||
                            rowCheat == CH_CFG_HK1  || rowCheat == CH_CFG_HK2);
            if (isCycler && (down & (BUTTON_LEFT | BUTTON_RIGHT)))
            {
                CfgCycle(rowCheat, (down & BUTTON_RIGHT) ? 1 : -1);
                changed = 1;
            }
            else
            {
            if (down & (BUTTON_RIGHT | BUTTON_R1)) // page down, clamped to the last row
            {
                int c = cursor;
                for (int k = 0; k < MAX_ROWS && c + 1 < fld->count; ++k) c++;
                while (c < fld->count && NavSkip(folderIdx, c)) c++;   // land on a selectable row
                if (c >= fld->count) c = cursor;                       // ran off the end: stay put
                if (c != cursor) { cursor = c; changed = 1; }
            }
            if (down & (BUTTON_LEFT | BUTTON_L1))  // page up, clamped to the first row
            {
                int c = cursor;
                for (int k = 0; k < MAX_ROWS && c > 0; ++k) c--;
                while (c > 0 && NavSkip(folderIdx, c)) c--;
                if (c != cursor) { cursor = c; changed = 1; }
            }
            }
        }

        if (down & BUTTON_A)
        {
            const Item *it = &fld->items[cursor];
            if (it->folder >= 0)
            {
                fstack[depth] = folderIdx; cstack[depth] = cursor; sstack[depth] = scroll; depth++;
                folderIdx = it->folder; cursor = 0; scroll = 0;
                // land on the first selectable row so the very first frame shows the highlight
                // (a folder like Settings opens on a section-header separator otherwise)
                { const Folder *nf = &folders[folderIdx];
                  while (cursor < nf->count && NavSkip(folderIdx, cursor)) cursor++;
                  if (cursor >= nf->count) cursor = 0; }
                changed = 1;
            }
            else if (it->warp == -2) // Teleport filter row: cycle All -> Overworld -> Dungeons
            { g_tpFilter = (g_tpFilter + 1) % 3; scroll = 0; changed = 1; }
            else if (it->warp >= 0) // teleport: write the entrance, then resume so the game loads it
            {
                if (MM_Warp(warps[it->warp].entrance))
                {
                    QueueToastRaw(T(warps[it->warp].name), T(": WARP"));
                    g_quitToGame = 1; // hand control back so the scene transition runs
                    break;
                }
                QueueToastRaw("Can't warp right now", "");
                changed = 1;
            }
            else if (it->picker >= 0)
            {
                PickerRun(&pickers[it->picker]);
                if (g_quitToGame) break; // SELECT inside picker -> straight to game
                prev = HID_PAD;
                changed = 1;
            }
            else if (it->tool >= 0)
            {
                ToolRun(it->tool);
                if (g_quitToGame) { g_resumeTool = it->tool; break; } // re-enter this tool on reopen
                ComposeBottom(); BotBlitComposeBoth(); // tool owned the bottom; restore menu legend
                prev = HID_PAD;
                changed = 1;
            }
            else if (it->cheat == CH_CFG_QMKEY || it->cheat == CH_CFG_HK1 || it->cheat == CH_CFG_HK2)
            {
                CfgCycle(it->cheat, 1); // A advances; left/right also cycle (shared with CfgCycle)
                changed = 1;
            }
            else if (it->cheat == CH_CFG_HKRESET)
            {
                qmCombo = 0; hk1 = HK1_DEFAULT; hk2 = HK2_DEFAULT; // L+SELECT / {Y} / {X}
                QueueToastRaw("Hotkeys reset to default", "");
                configDirty = 1;
                changed = 1;
            }
            else if (it->cheat == CH_CFG_THEME)
            {
                ThemePicker();
                if (g_quitToGame) break; // SELECT inside picker -> straight to game
                prev = HID_PAD;
                changed = 1;
            }
            else if (it->cheat == CH_CFG_LANG)
            {
                CfgCycle(CH_CFG_LANG, 1); // A advances; ←/→ also cycle (shared with CfgCycle)
                changed = 1;
            }
            else if (OneShot(it->cheat))
            {
                char sfx[48]; sniprintf(sfx, sizeof sfx, ": %s", g_oneShotMsg);
                QueueToastRaw(T(it->label), sfx);
                flashMsg = g_oneShotMsg;
                flashCheat = it->cheat; flashTicks = 50; // ~0.8s feedback
                changed = 1;
            }
            else
            {
                cheatState[it->cheat] ^= 1;
                if (it->cheat == CH_CFG_TOAST || it->cheat == CH_CFG_AUTOFILL) configDirty = 1; // settings toggles: persist, no self-toast
                else QueueToast(T(it->label), cheatState[it->cheat]);
                changed = 1;
            }
        }

        if (down & BUTTON_Y)
        {
            const Item *it = &fld->items[cursor];
            if (it->folder >= 0) // star a folder -> quick menu shortcut that opens it
            { folderFav[it->folder] ^= 1; favDirty = 1; changed = 1; }
            else if (it->tool >= 0) // star a tool -> quick menu shortcut that launches it
            { toolFav[it->tool] ^= 1; favDirty = 1; changed = 1; }
            else if (it->folder < 0 && it->picker < 0 && it->tool < 0 &&
                it->cheat >= 0 && it->cheat != CH_CFG_QMKEY && it->cheat != CH_CFG_THEME &&
                it->cheat != CH_CFG_LANG && it->cheat != CH_CFG_HK1 && it->cheat != CH_CFG_HK2 &&
                it->cheat != CH_CFG_HKRESET && it->cheat != CH_CFG_TOAST && it->cheat != CH_CFG_AUTOFILL)
            { favorite[it->cheat] ^= 1; favDirty = 1; changed = 1; }
        }

        if (down & BUTTON_X)
        {
            const Item *it = &fld->items[cursor];
            if (it->desc || it->warp >= 0) // warp rows carry their desc in warps[], not here
            {
                InfoBox(it);
                if (g_quitToGame) break; // SELECT dismissed the info box -> to game
                prev = HID_PAD;
                changed = 1;
            }
        }

        if (down & BUTTON_B)
        {
            if (depth > 0) { depth--; folderIdx = fstack[depth]; cursor = cstack[depth]; scroll = sstack[depth]; changed = 1; }
            else break;
        }
        if (down & BUTTON_SELECT) break;

        if (flashTicks > 0 && --flashTicks == 0) { flashCheat = -1; changed = 1; }

        if (folderIdx == F_ROOT || (folderIdx == F_TELEPORT && g_tpFilter == 0)) // 2-col grid: scroll is a pixel offset that follows the cursor
        {
            BuildRootLayout(fld);
            int visBot = WIN_Y + WIN_H - 22, cy = g_rlY[cursor];
            if (cy - scroll < ROW_Y0)         scroll = cy - ROW_Y0;
            if (cy + ROW_H - scroll > visBot) scroll = cy + ROW_H - visBot;
            if (scroll < 0) scroll = 0;
        }
        else
        {
            int cvp = VisPos(folderIdx, cursor); // scroll tracks the cursor's VISIBLE position
            if (cvp < scroll)             scroll = cvp;
            if (cvp >= scroll + MAX_ROWS) scroll = cvp - MAX_ROWS + 1;
        }

        if (changed) { ComposeMenu(&folders[folderIdx], depth, cursor, scroll); Present(); }
    }
    } // end if (!g_quitToGame)

    flashCheat = -1; flashTicks = 0;
    menuDepth = depth; menuFolder = folderIdx; menuCursor = cursor; menuScroll = scroll;

    BotRestoreBoth();
    TopRelease();   // hand the top screen back too, else it stays frozen on our last frame
    DrainButtons(BUTTON_B | BUTTON_SELECT | BUTTON_A); // let go of B before the game sees it (else: sword swing)
    ResumeGame();

    if (configDirty) { ConfigSave(); configDirty = 0; } // write after resuming (fs is slow)
    if (favDirty)    { FavSave();    favDirty = 0; }
}

// ===================== Quick menu (favorites, L+SELECT) =====================
static void QuickMenu(void)
{
    struct { const char *label; int cheat; const Item *it; char sl[40]; } ent[12];
    int n = 0;
    for (int f = 0; f < NUM_FOLDERS; ++f)
        for (int i = 0; i < folders[f].count && n < 12; ++i)
        {
            const Item *it = &folders[f].items[i];
            if (it->cheat >= 0 && it->folder < 0 && it->picker < 0 && it->tool < 0 && favorite[it->cheat])
            {
                ent[n].label = it->label; ent[n].cheat = it->cheat; ent[n].it = it;
                // short label: drop any " (...)" tail so the panel stays narrow (X shows the full info)
                const char *L = T(it->label); int k = 0;
                while (L[k] && k < 39 && !(L[k] == ' ' && L[k + 1] == '(')) { ent[n].sl[k] = L[k]; k++; }
                ent[n].sl[k] = 0;
                n++;
            }
            else if (it->folder >= 0 && folderFav[it->folder]) // starred folder shortcut
            {
                ent[n].label = it->label; ent[n].cheat = -1; ent[n].it = it;
                const char *L = T(it->label); int k = 0;
                while (L[k] && k < 39) { ent[n].sl[k] = L[k]; k++; }
                ent[n].sl[k] = 0;
                n++;
            }
            else if (it->tool >= 0 && toolFav[it->tool]) // starred tool shortcut
            {
                ent[n].label = it->label; ent[n].cheat = -1; ent[n].it = it;
                const char *L = T(it->label); int k = 0;
                while (L[k] && k < 39) { ent[n].sl[k] = L[k]; k++; }
                ent[n].sl[k] = 0;
                n++;
            }
        }

    if (!gCompose) return;
    SysFontInit();   // idempotent - ensures the system font is loaded even when the quick menu is
                     // the FIRST thing opened after boot (else info boxes fall back to the tiny font)
    PauseGame();
    TopTakeOver();
    GrabFb();
    CaptureTopBackdrop(); // save the game frame so we can repaint cleanly (e.g. after the X info box)

    // Compact panel: small 6x10 font, short rows. Icons are the full 16px DrawCheatIcon (same as
    // the main menu) so the hand-drawn vector icons (moon, pin, portal, ...) show here too - the
    // old path only blitted real sprites via FindSprite, so vector-icon cheats had a blank slot.
    // Column layout: checkbox @+6, icon @+18 (16px), label @+38 (relative to panel x)
    #define QM_X   8
    #define QM_Y   8
    #define QM_RH  17
    #define QM_LBL 38
    int rows = n ? n : 1;
    int w = QM_LBL + C6Width("Favorites") + 6;
    for (int i = 0; i < n; ++i)
    {
        int lw = QM_LBL + C6Width(ent[i].sl) + 6;
        if (lw > w) w = lw;
    }
    if (!n)
    {
        int lw = 7 + C6Width("Press Y in the menu to star") + 6;
        if (lw > w) w = lw;
    }
    if (w > 384 - 16) w = 384 - 16;
    int h = 20 + rows * QM_RH + 6;

    DrainButtons(BUTTON_SELECT);   // capped: a stuck pad must not hang the console with the game paused
    u32 prev = HID_PAD;
    static int qmLastCursor = 0;              // reopen on the entry you last had selected
    int cursor = (qmLastCursor < n) ? qmLastCursor : (n > 0 ? n - 1 : 0);
    int changed = 1;

    while (1)
    {
        if (changed)
        {
            // auto-contrast: light theme bg -> dark text, dark bg -> light text (keeps every theme readable)
            int bgLight = ThemeBgLight();
            u8 tR = bgLight ? 28 : 238, tG = bgLight ? 26 : 236, tB = bgLight ? 30 : 224;   // off label
            u8 gR = bgLight ? 22 : 150, gG = bgLight ? 108 : 236, gB = bgLight ? 30 : 130;  // on label (green)
            u8 aR = bgLight ? 150 : CGOLD[0], aG = bgLight ? 100 : CGOLD[1], aB = bgLight ? 10 : CGOLD[2]; // gold accent
            RestoreTopBackdrop(); // repaint the game frame (clears any prior info box / stale box)
            CFill(QM_X, QM_Y, w, h, BG);
            CFill(QM_X, QM_Y, w, 1, aR, aG, aB); CFill(QM_X, QM_Y + h - 1, w, 1, aR, aG, aB);
            CFill(QM_X, QM_Y, 1, h, aR, aG, aB); CFill(QM_X + w - 1, QM_Y, 1, h, aR, aG, aB);
            StarIcon(QM_X + 5, QM_Y + 4);
            CText6(QM_X + 15, QM_Y + 4, T("Favorites"), aR, aG, aB);
            CFill(QM_X + 5, QM_Y + 15, C6Width("Favorites") + 12, 1, aR, aG, aB);

            if (!n) CText6(QM_X + 7, QM_Y + 22, T("Press Y in the menu to star"), tR, tG, tB);
            for (int i = 0; i < n; ++i)
            {
                int y = QM_Y + 20 + i * QM_RH;
                if (i == cursor)
                {
                    CFillBlend(QM_X + 3, y - 1, w - 6, QM_RH, bgLight ? 255 : 0, bgLight ? 255 : 0, bgLight ? 255 : 0, 90);
                    CFill(QM_X + 3, y - 1, 2, QM_RH, aR, aG, aB);
                }
                if (ent[i].it->folder >= 0) // folder shortcut: tinted box + category icon, opens the folder
                {
                    BrownBoxS(QM_X + 6, y + 3);
                    CategoryIcon(ent[i].it->folder, QM_X + 18, y);
                    CText6(QM_X + QM_LBL, y + 4, ent[i].sl, aR, aG, aB); // accent = "does something"
                }
                else if (ent[i].it->tool >= 0) // tool shortcut: tinted box + its icon, launches the tool
                {
                    BrownBoxS(QM_X + 6, y + 3);
                    ToolIcon(ent[i].it->tool, QM_X + 18, y);
                    CText6(QM_X + QM_LBL, y + 4, ent[i].sl, aR, aG, aB);
                }
                else
                {
                int on = cheatState[ent[i].cheat] || flashCheat == ent[i].cheat;
                if (IsToggleCheat(ent[i].cheat)) CheckBoxIconS(QM_X + 6, y + 3, on); // on/off toggle -> checkbox
                else                             BrownBoxS(QM_X + 6, y + 3);         // one-shot action -> tinted box
                DrawCheatIcon(QM_X + 18, y, ent[i].cheat);
                if (flashCheat == ent[i].cheat)
                {
                    int fx = QM_X + w - 6 - C6Width(flashMsg);
                    if (flashMsg[0] == 'R') CText6(fx, y + 4, flashMsg, aR, aG, aB);
                    else                    CText6(fx, y + 4, flashMsg, gR, gG, gB);
                }
                else
                    CText6(QM_X + QM_LBL, y + 4, ent[i].sl, on ? gR : tR, on ? gG : tG, on ? gB : tB);
                }
            }
            Present(); Present();
            changed = 0;
        }

        svcSleepThread(16 * 1000 * 1000);
        u32 pad = HID_PAD, down = ARepeat(pad, &prev, &g_arHold);

        if ((down & BUTTON_DOWN) && n) { cursor = (cursor + 1 < n) ? cursor + 1 : 0; changed = 1; }
        if ((down & BUTTON_UP)   && n) { cursor = (cursor > 0) ? cursor - 1 : n - 1; changed = 1; }
        if ((down & BUTTON_A) && n)
        {
            if (ent[cursor].it->folder >= 0) // folder shortcut: request opening it, then close the quick menu
            { g_openFolder = ent[cursor].it->folder; break; }
            else if (ent[cursor].it->tool >= 0) // tool shortcut: request launching it, then close the quick menu
            { g_openTool = ent[cursor].it->tool; break; }
            else if (OneShot(ent[cursor].cheat))
            {
                char sfx[48]; sniprintf(sfx, sizeof sfx, ": %s", g_oneShotMsg);
                QueueToastRaw(ent[cursor].label, sfx);
                flashMsg = g_oneShotMsg;
                flashCheat = ent[cursor].cheat; flashTicks = 50;
                changed = 1;
            }
            else
            {
                cheatState[ent[cursor].cheat] ^= 1;
                QueueToast(ent[cursor].label, cheatState[ent[cursor].cheat]);
                changed = 1;
            }
        }
        if ((down & BUTTON_Y) && n) // unfavorite the selected entry and drop it from the list
        {
            if (ent[cursor].it->folder >= 0)      folderFav[ent[cursor].it->folder] = 0;
            else if (ent[cursor].it->tool >= 0)   toolFav[ent[cursor].it->tool] = 0;
            else                                  favorite[ent[cursor].cheat] = 0;
            favDirty = 1;
            for (int j = cursor; j < n - 1; ++j) ent[j] = ent[j + 1];
            n--;
            if (cursor >= n) cursor = n > 0 ? n - 1 : 0;
            changed = 1;
        }
        if ((down & BUTTON_X) && n && ent[cursor].it->desc) // open the info box
        {
            InfoBox(ent[cursor].it);
            if (g_quitToGame) break; // SELECT dismissed the info box -> to game
            prev = HID_PAD;
            changed = 1;
        }
        if (flashTicks > 0 && --flashTicks == 0) { flashCheat = -1; changed = 1; }
        if (down & (BUTTON_B | BUTTON_SELECT)) break;
    }

    qmLastCursor = cursor; // remember where we were, for the next open
    flashCheat = -1; flashTicks = 0;
    DrainButtons(BUTTON_B | BUTTON_SELECT | BUTTON_A);
    // Only hand back if we are really going to the game. When a favorite folder or tool was
    // picked, RunMenu() takes over right after - handing back here would show one frame of the
    // game just to recapture it, which is the flicker this hand-off exists to avoid.
    if (g_openFolder < 0 && g_openTool < 0) TopRelease();
    ResumeGame();
    if (favDirty)  { FavSave(); favDirty = 0; }  // persist changes made from the quick menu too
}

// ===================== Thread / entry =====================

// Release everything we hold, in the reverse order we took it. Called when Luma tells us the
// game is exiting, BEFORE we signal that the loader may continue.
__attribute__((unused)) static void PluginShutdown(void)
{
    // If we are torn down with the menu open the game's threads are still suspended. Never let
    // a process try to exit with its own threads frozen.
    ResumeGame();

    // PROOF that this path ran. There is no screen to draw on at exit, so drop a marker file
    // next to the .3gx instead. If it appears after closing the game, Luma really did signal
    // onProcessExitEvent and the handshake is live; if it never appears, the event was never
    // delivered and the whole block is dead weight. Cheap, and it settles the question.
    if (fsReady)
    {
        Handle f;
        if (R_SUCCEEDED(FSUSER_OpenFile(&f, cfgArchive,
                fsMakePath(PATH_ASCII, PlgPath("exit_handshake_ran.txt")),
                FS_OPEN_WRITE | FS_OPEN_CREATE, 0)))
        {
            static const char msg[] = "PluginShutdown() ran; resumeExitEvent was signalled.\n";
            u32 wrote = 0;
            FSFILE_SetSize(f, sizeof(msg) - 1);
            FSFILE_Write(f, &wrote, 0, msg, sizeof(msg) - 1, FS_WRITE_FLUSH);
            FSFILE_Close(f);
        }
    }

    // Last chance to persist anything the user changed and we had not written yet.
    if (configDirty) { ConfigSave(); configDirty = 0; }
    if (favDirty)    { FavSave();    favDirty = 0; }

    if (hidShmem && g_hidMem)          // touch-panel shared memory (manual hid:USER init)
    {
        svcUnmapMemoryBlock(g_hidMem, (u32)hidShmem);
        svcCloseHandle(g_hidMem);
        g_hidMem = 0; hidShmem = NULL; hidReady = 0;
    }
    if (fsReady)                            // SD archive + fs session
    {
        FSUSER_CloseArchive(cfgArchive);
        fsExit();
        fsReady = 0;
    }
    plgLdrExit();
}

void ThreadMain(void *arg)
{
    (void)arg;        // the loader's thread signature passes one; this plugin has no use for it
    InitThreadVars(); // must run before any newlib/hid/fs call on this thread

    // Make the whole game process RWX up front, exactly like CTRPluginFramework does at init.
    // Many ported cheats write to the game's read-only segments (const tables in .rodata, code in
    // .text). Under CTRPF those writes worked *only* because CTRPF had already flipped the process
    // to RWX globally; a raw plugin defaults to RO there, so those writes silently no-op. Doing the
    // same flip once here restores the CTRPF behavior for every cheat at once (Can Use All Items,
    // and any other .rodata/.text write), instead of patching RWX in one cheat at a time. This can
    // only ENABLE previously-failing writes to RO pages - writes to already-writable RAM are
    // unaffected - so it never breaks a cheat that already worked.
    svcControlProcess(CUR_PROCESS_HANDLE, PROCESSOP_SET_MMU_TO_RWX, 0, 0);

    gCompose = (u8 *)malloc(TOP_W * TOP_H * 3);
    savedBot = (u16 *)malloc(BOT_W * BOT_H * 2);
    savedTop = (u16 *)malloc(TOP_W * TOP_H * 2);
    ApplyTheme(0); // seed the live colors from THEMES[0] BEFORE anything can draw. Without this a
                   // fresh install (no Settings.cfg -> ConfigLoad never calls ApplyTheme) would run
                   // on whatever the CINK/CBG initializers happen to hold.
    ConfigLoad(); // restore toast toggle + quick-menu hotkey + theme + language from SD
    FavLoad();    // restore favorites (own label-keyed file, survives cheat-list changes)

    u32 prev = HID_PAD;
    int comboPrev = 0;
    while (1)
    {
        // 4ms while a toast is on screen (fast re-stamp), 20ms otherwise
        svcSleepThread((toastTicks > 0 ? 4 : 20) * 1000 * 1000);

#if EXIT_HANDSHAKE
        // Luma signals onProcessExitEvent when the game is shutting down. The 3gx contract
        // appears to be: clean up, then signal resumeExitEvent so the loader can finish tearing
        // the plugin down.
        //
        // DISABLED BY DEFAULT because it was TESTED AND DOES NOT RUN. Built with this on, and
        // with PluginShutdown() writing a marker file to the SD card as proof, the marker never
        // appeared after closing the game - while Settings.cfg written by the same code path
        // did. So the file I/O was fine and this block simply never executed:
        // PROCESSOP_GET_ON_EXIT_EVENT in main() does not hand back a usable event, and
        // onProcessExitEvent stays 0.
        //
        // The reference plugin this engine came from ignores these events too, and shows no
        // teardown problems, so nothing is lost. Kept behind the flag in case a future Luma
        // build starts delivering the event - the marker file makes that a one-run check.
        if (onProcessExitEvent && svcWaitSynchronization(onProcessExitEvent, 0) == 0)
        {
            PluginShutdown();
            if (resumeExitEvent) svcSignalEvent(resumeExitEvent);
            svcExitThread();   // does not return
        }
#endif
        u32 pad = HID_PAD, down = ARepeat(pad, &prev, &g_arHold);

        const QmCombo *qc = &qmCombos[qmCombo];
        int comboNow = (pad & qc->pad) == qc->pad;

        if (comboNow && !comboPrev)
        {
            QuickMenu();
            prev = HID_PAD;
            comboNow = 1; // treat as still held: no instant reopen
            if (g_openFolder >= 0) // a folder shortcut was picked in the quick menu -> open it
            {
                int fld = g_openFolder; g_openFolder = -1;
                // Save the normal menu position and restore it afterwards, so this transient jump
                // doesn't hijack where SELECT reopens (else SELECT would keep landing in this folder).
                int sD = menuDepth, sF = menuFolder, sC = menuCursor, sS = menuScroll;
                menuDepth = 0; menuFolder = fld; menuScroll = 0; menuCursor = 0;
                { const Folder *nf = &folders[fld]; // land on the first selectable row
                  while (menuCursor < nf->count && IS_SEP(&nf->items[menuCursor])) menuCursor++;
                  if (menuCursor >= nf->count) menuCursor = 0; }
                g_qmHandoff = 1;   // the quick menu is still on screen: do not recapture it as backdrop
                RunMenu();
                menuDepth = sD; menuFolder = sF; menuCursor = sC; menuScroll = sS;
                prev = HID_PAD;
            }
            else if (g_openTool >= 0) // a tool shortcut was picked in the quick menu -> launch it
            {
                int t = g_openTool; g_openTool = -1;
                int sD = menuDepth, sF = menuFolder, sC = menuCursor, sS = menuScroll;
                g_resumeTool = t;                    // RunMenu's resume path runs the tool with full setup
                menuDepth = 0; menuFolder = 0; menuCursor = 0; menuScroll = 0; // land on HOME after the tool
                { const Folder *nf = &folders[F_ROOT]; // first selectable row, not a separator
                  while (menuCursor < nf->count && IS_SEP(&nf->items[menuCursor])) menuCursor++;
                  if (menuCursor >= nf->count) menuCursor = 0; }
                g_qmHandoff = 1;   // the quick menu is still on screen: do not recapture it as backdrop
                RunMenu();
                menuDepth = sD; menuFolder = sF; menuCursor = sC; menuScroll = sS;
                prev = HID_PAD;
            }
        }
        else if ((down & BUTTON_SELECT) && !comboNow)
        {
            RunMenu();
            prev = HID_PAD;
        }
        comboPrev = comboNow;

        ApplyCheats();
        ToastTick();
    }
}

// Normally provided by the 3dsx crt0; NULL = "no homebrew env, use real srv"
void *__service_ptr = NULL;

extern char* fake_heap_start;
extern char* fake_heap_end;
extern u32 __ctru_heap;
extern u32 __ctru_linear_heap;
u32 __ctru_heap_size = 0;
u32 __ctru_linear_heap_size = 0;

void __system_allocateHeaps(PluginHeader *header)
{
    __ctru_heap_size = header->heapSize;
    __ctru_heap = header->heapVA;
    fake_heap_start = (char *)__ctru_heap;
    fake_heap_end = fake_heap_start + __ctru_heap_size;
}

void main(void)
{
    PluginHeader *header = (PluginHeader *)0x07000000;
    if (header->magic != HeaderMagic) return;
    __system_allocateHeaps(header);
    cheatState[CH_CFG_TOAST] = 1;    // notifications on by default
    cheatState[CH_CFG_AUTOFILL] = 1; // auto-fill the Checklist on open by default
    srvInit();
    plgLdrInit();
    svcControlProcess(CUR_PROCESS_HANDLE, PROCESSOP_GET_ON_EXIT_EVENT, (u32)&onProcessExitEvent, (u32)&resumeExitEvent);
    svcCreateThread(&thread, ThreadMain, 0, (u32 *)(stack + PLG_STACK_SIZE), 30, -1);
}
