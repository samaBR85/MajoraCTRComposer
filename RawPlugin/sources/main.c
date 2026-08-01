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
#include "engine/guide_reader.inc.c"

#if !TOOLS_ONLY
#include "plugin/guide_text.inc.c"
#include "engine/guide_game.inc.c"
#endif

#include "engine/guide_plugin.inc.c"

#if !TOOLS_ONLY
#include "engine/tracker.inc.c"
#include "plugin/tracker_data.inc.c"
#include "engine/tracker_ui.inc.c"
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
