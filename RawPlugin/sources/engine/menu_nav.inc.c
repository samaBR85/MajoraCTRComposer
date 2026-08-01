static u8  folderFav[NUM_FOLDERS]; // folders starred for the quick menu (own Favorites lines, '#'-prefixed)
static u8  toolFav[NUM_TOOLS];     // tools starred for the quick menu (own Favorites lines, '&'-prefixed)
static int g_openFolder = -1;      // quick menu sets this to a folder id to open after it closes
static int g_openTool   = -1;      // quick menu sets this to a tool id to launch after it closes
// Set when RunMenu() is called straight from the quick menu. Means the game has NOT drawn a frame
// since we last painted the screen, so GrabFb() would capture our own panel and bake it into the
// backdrop - the menu renders over a photo of itself, and each reopen stacks another copy.
static int g_qmHandoff = 0;
// stable keys for tool favorites in Favorites.txt (index = tool id; order must match the T_* enum)
static const char *kToolKeys[NUM_TOOLS] = {
#if TOOLS_ONLY
    "Cheat Search", "RAM Dumper", "Hex Editor", "About", "Plugin Guide"
#else
    "Cheat Search", "RAM Dumper", "Hex Editor", "About", "Game Guide", "Plugin Guide", "Tracker"
#endif
};

static int g_tpFilter = 0; // Teleport category filter: 0=all, 1=overworld, 2=dungeons, 3=owl statues

// An item is hidden when the Teleport filter excludes its category. Only ever true inside
// F_TELEPORT; every other folder shows everything, same as the template default.
static int ItemHidden(int folderIdx, const Item *it)
{
    if (folderIdx != F_TELEPORT || g_tpFilter == 0) return 0;
    if (it->warp >= 1)
    {
        const Warp *w = &warps[it->warp];
        if (g_tpFilter == 1) return w->isDungeon;   // Overworld: hide dungeons
        if (g_tpFilter == 2) return !w->isDungeon;  // Dungeons: hide everything else
        return !w->isOwl;                            // Owl Statues: hide everything else
    }
    if (IS_SEP(it)) return 0; // keep section headers visible regardless of filter
    return 0;
}
// A row the cursor must skip over: a section header OR a filtered-out item.
static int NavSkip(int folderIdx, int c)
{ const Folder *f = &folders[folderIdx]; return IS_SEP(&f->items[c]) || ItemHidden(folderIdx, &f->items[c]); }
// Visible position (0-based) of a raw item index, skipping hidden items - used for scroll math.
static int VisPos(int folderIdx, int rawIdx)
{ int vp = 0; const Folder *f = &folders[folderIdx];
  for (int i = 0; i < f->count && i < rawIdx; ++i) if (!ItemHidden(folderIdx, &f->items[i])) vp++;
  return vp; }
