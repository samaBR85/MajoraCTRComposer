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

#include "engine/render.inc.c"

#include "plugin/cheat_ids.inc.c"

#include "engine/storage.inc.c"

#include "plugin/cheats.inc.c"

#include "plugin/pickers.inc.c"

#include "engine/menu_model.inc.c"

#include "plugin/menu_tables.inc.c"

#include "engine/menu_nav.inc.c"

#include "engine/favorites.inc.c"


#include "engine/theme.inc.c"

#include "engine/sprites.inc.c"

#include "plugin/cheat_icons.inc.c"

#include "engine/icons.inc.c"

#include "engine/bottom_screen.inc.c"

#include "engine/toast.inc.c"

#include "engine/menu_render.inc.c"

#include "engine/tools.inc.c"
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
