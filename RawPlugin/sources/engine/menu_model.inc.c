// ===================== Menu model (folders) =====================
// A layout-agnostic data model: DrawMenuItem() renders ONE row/cell wherever you put it,
// so HOME can be a plain scrolling list, a 2-column grid, or anything else. This template
// uses the 2-column grid for HOME and simple lists everywhere else.
typedef struct { const char *label; int cheat; int folder; int picker; const char *desc; int tool; int warp; u8 wide; } Item;
typedef struct { const char *title; const Item *items; int count; } Folder;

#if TOOLS_ONLY
enum { F_ROOT, F_SETTINGS, NUM_FOLDERS };
#else
enum { F_ROOT, F_TIME, F_BATTLE, F_INVENTORY, F_AMMO, F_BOTTLES, F_QUEST, F_MISC, F_TELEPORT, F_EXAMPLES, F_TOOLS, F_SETTINGS, NUM_FOLDERS };
#endif

// tool screens. The tools-only build drops the two that are inherently per-game, so their
// code, their data tables and their menu rows all disappear from that binary.
#if TOOLS_ONLY
enum { T_SEARCH, T_RAMDUMP, T_HEXEDIT, T_ABOUT, T_PLUGINGUIDE, NUM_TOOLS };
#else
enum { T_SEARCH, T_RAMDUMP, T_HEXEDIT, T_ABOUT, T_GAMEGUIDE, T_PLUGINGUIDE, T_TRACKER, NUM_TOOLS };
#endif
static void ToolRun(int t); // fwd

#define IT_CHEAT(lbl, ch, d)   { lbl, ch, -1, -1, d, -1, -1, 0 }
#define IT_FOLDER(lbl, fl)     { lbl, -1, fl, -1, NULL, -1, -1, 0 }
#define IT_PICKER(lbl, pk, d)  { lbl, -1, -1, pk, d, -1, -1, 0 }
#define IT_TOOL(lbl, tl, d)    { lbl, -1, -1, -1, d, tl, -1, 0 }
#define IT_TOOL_WIDE(lbl, tl, d) { lbl, -1, -1, -1, d, tl, -1, 1 } // HOME only: spans both columns, still selectable
#define IT_WARP(lbl, wp, d)    { lbl, -1, -1, -1, d, -1, wp, 0 }  // teleport destination (index into warps[])
#define IT_WARP_WIDE(lbl, wp, d) { lbl, -1, -1, -1, d, -1, wp, 1 } // full-width teleport row
#define IT_TPFILTER            { "Filter", -1, -1, -1, NULL, -1, -2, 1 } // Teleport category filter (warp==-2, wide)
#define IT_SEP(lbl)            { lbl, -2, -1, -1, NULL, -1, -1, 0 } // non-selectable section header
#define IS_SEP(it)             ((it)->cheat == -2)
