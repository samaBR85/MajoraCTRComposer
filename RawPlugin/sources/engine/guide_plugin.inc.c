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
#endif

#define PLUGIN_NPAGES ((int)(sizeof(PLUGIN_PAGES) / sizeof(PLUGIN_PAGES[0])))

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
