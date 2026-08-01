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
