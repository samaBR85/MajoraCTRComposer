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
        const char *m = g_tpFilter == 1 ? T("Overworld") : g_tpFilter == 2 ? T("Dungeons") :
                         g_tpFilter == 3 ? T("Owl Statues") : T("All");
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
                CText6(ROW_X, dy + 3, sec, INK_DIM); // matches the list-mode header style (DrawMenuItem)
                int lx = ROW_X + C6Width(sec) + 6;
                CFill(lx, dy + 6, (WIN_X + WIN_W - 14) - lx, 1, GOLD); // hairline rule
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
