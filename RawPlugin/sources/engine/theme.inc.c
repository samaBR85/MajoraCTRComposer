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
        case CH_MM_ISOT_THIRD: case CH_MM_MOONJUMP:
        case CH_MG_TOWN_GALLERY: case CH_MG_SWAMP_GALLERY: case CH_MG_BEAVER:
        case CH_MG_BOAT_JUMP: case CH_MG_HONEY_DARLING:
        case CH_MV_DEKU_WALK: case CH_MV_ZORA_SWIM:
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
