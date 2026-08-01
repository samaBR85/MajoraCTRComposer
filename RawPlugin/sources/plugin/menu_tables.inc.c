#if TOOLS_ONLY
// Universal build: the tools ARE the plugin, so they go straight on HOME with no folder to
// dig through. No cheats, no tracker, no game guide - none of those can mean anything when the
// same binary loads into every title on the system.
static const Item rootItems[] = {
    IT_SEP("MEMORY TOOLS"),
    IT_TOOL_WIDE("Cheat Search", T_SEARCH,  "Search this game's RAM for a value, then narrow it down (greater/less/changed...) to find its address. Poke results directly. Works on any title - it scans memory, it doesn't need to know the game."),
    IT_TOOL("RAM Dumper",   T_RAMDUMP, "Save a block of memory to a .bin on the SD card. Pick a start address and size, or pull the address from Cheat Search."),
    IT_TOOL("Hex Editor",   T_HEXEDIT, "Browse memory as a live hex grid and edit any byte on the spot. Read-only regions are protected."),
    IT_SEP("SYSTEM"),
    IT_TOOL("Plugin Guide", T_PLUGINGUIDE, "How to use this plugin: the menu, the quick menu, and the memory tools."),
    IT_TOOL("About",        T_ABOUT,   "Plugin info and credits."),
    IT_FOLDER("Settings",   F_SETTINGS),
};
#else
static const Item rootItems[] = {
    IT_SEP("CHEATS"),
    IT_FOLDER("Time", F_TIME),
    IT_FOLDER("Battle", F_BATTLE),
    IT_FOLDER("Inventory", F_INVENTORY),
    IT_FOLDER("Quest", F_QUEST),
    IT_FOLDER("Misc.", F_MISC),
    IT_FOLDER("Teleport", F_TELEPORT),
    IT_SEP("GUIDES"),
    IT_TOOL_WIDE("100% Checklist", T_TRACKER, "A 100% progress tracker: Masks, Stray Fairies and Gear upgrades. Each entry is untouched / auto / checked / cleared. Auto-fill syncs it from game memory - none of it is hardware-confirmed yet, so double-check against your own save before trusting it."),
    IT_TOOL("Game Guide",   T_GAMEGUIDE,   "A scrollable, categorized reader for your game's content. Ships with placeholder pages - replace them, or drop guide/English/game.txt on the SD card."),
    IT_TOOL("Plugin Guide", T_PLUGINGUIDE, "How to use this plugin: the menu, the quick menu, and the Cheat Search / RAM Dumper / Hex Editor tools."),
    IT_SEP("SYSTEM"),
    IT_FOLDER("Tools",    F_TOOLS),
    IT_FOLDER("Settings", F_SETTINGS),
};
#endif
#if !TOOLS_ONLY
static const Item toolsItems[] = {
    IT_TOOL("Cheat Search", T_SEARCH,  "Search the game's RAM for a value, then narrow it down (greater/less/changed...) to find its address. Poke results directly."),
    IT_TOOL("RAM Dumper",   T_RAMDUMP, "Save a block of the game's memory to a .bin file on the SD card. Pick a start address and size, or pull the address from Cheat Search."),
    IT_TOOL("Hex Editor",   T_HEXEDIT, "Browse memory as a live hex grid and edit any byte on the spot. Jump to an address, or to your Cheat Search result. Read-only regions are protected."),
    IT_TOOL("About",        T_ABOUT,   "Plugin info and credits."),
};

// EXAMPLE cheats - these demonstrate the shapes a cheat can take. Delete them and write your
// own; the descriptions are what the info box ({X}) shows.
// EVERY row here is INERT: EXAMPLE_ENABLED is 0, so toggling them writes nothing at all.
// They exist so you can walk the menu - navigation, auto-repeat, the {X} info box, {Y}
// favorites, toasts, the checkbox-vs-action distinction - before you have a single address.
static const Item exampleItems[] = {
    IT_SEP("CONTINUOUS (toggles)"),
    IT_CHEAT("Example: direct write",  CH_EX_DIRECT,
             "EXAMPLE - inert until you edit it. Writes a fixed 16-bit value to a fixed address every frame while it is on. The simplest kind of cheat: see EXAMPLE_ADDR_DIRECT in Sources/main.c."),
    IT_CHEAT("Example: byte write",    CH_EX_BYTE,
             "EXAMPLE - inert until you edit it. Same idea, but 8-bit. Match the write width (W8 / W16 / W32) to whatever the game actually stores at that address, or you will clobber the bytes next door."),
    IT_CHEAT("Example: 32-bit write",  CH_EX_WORD,
             "EXAMPLE - inert until you edit it. A 32-bit write, for counters and pointers that are a full word wide."),
    IT_CHEAT("Example: base + offset", CH_EX_BASEOFF,
             "EXAMPLE - inert until you edit it. Reads a pointer to the player struct, then writes a field at a fixed offset inside it. ALWAYS null-check the base before writing through it."),
    IT_CHEAT("Example: hold {HK}",     CH_EX_HOTKEY,
             "EXAMPLE - inert until you edit it. Only acts while you hold {HK} in game. Rebind that button in Settings - this text shows the live binding, because the token is swapped for the real glyph when the card opens."),
    IT_SEP("ONE-SHOT (actions)"),
    IT_CHEAT("Example: apply once",    CH_EX_ONESHOT,
             "EXAMPLE - inert until you edit it. Applied once, the moment you press {A}, instead of every frame. Use this for 'give me the item' style cheats. Note it gets a plain box, not a checkbox: it has no on/off state."),
    IT_CHEAT("Example: toggle a bit",  CH_EX_ONESHOT2,
             "EXAMPLE - inert until you edit it. A one-shot that flips a bit and then reads it back, so the flash says ADDED or REMOVED instead of just OK. Good for equipment-style cheats."),
};

// MM3D Time cheats. Addresses verified against the offline save-file map (SaveGames/, anchored
// at RAM = file_offset + 0x7761D8) and cross-checked against the AR code list in
// references/mm3d-ar-cheats-usa-0004000000125500.txt. USA, v1.1.0 (Title ID 0004000000125500).
// One-shots: pressing {A} writes once, it does not hold the value.
static const Item timeItems[] = {
    IT_SEP("DAY"),
    IT_CHEAT("Set to Day 1", CH_MM_DAY1, "CONFIRMED on hardware: takes effect on your next area transition (door, warp, load), not instantly. Address 0x7761EC, u8."),
    IT_CHEAT("Set to Day 2", CH_MM_DAY2, "CONFIRMED on hardware: takes effect on your next area transition (door, warp, load), not instantly. Address 0x7761EC, u8."),
    IT_CHEAT("Set to Day 3", CH_MM_DAY3, "CONFIRMED on hardware: takes effect on your next area transition (door, warp, load), not instantly. Address 0x7761EC, u8."),
    IT_SEP("TIME OF DAY"),
    IT_CHEAT("Set Time to 6AM",  CH_MM_TIME_6AM,  "CONFIRMED on hardware: applies instantly. KNOWN LIMITATION: jumping to an EARLIER time than now also advances the day - the game treats the decrease as midnight passing. Not currently fixable with a simple write. Address 0x7761F9, u8 = 0x40."),
    IT_CHEAT("Set Time to 10AM", CH_MM_TIME_10AM, "CONFIRMED on hardware: applies instantly. KNOWN LIMITATION: jumping to an EARLIER time than now also advances the day - the game treats the decrease as midnight passing. Not currently fixable with a simple write. Address 0x7761F9, u8 = 0x6B."),
    IT_CHEAT("Set Time to 6PM",  CH_MM_TIME_6PM,  "CONFIRMED on hardware: applies instantly. Address 0x7761F9, u8 = 0xC0."),
    IT_SEP("SCRUB (hold + D-Pad)"),
    IT_CHEAT("Time Scrub",  CH_MM_TIME_SCRUB, "CONFIRMED on hardware. Hold {HK} + D-Pad Up/Down to nudge the clock forward/backward while held. Rebind the hold button in Settings. Forward is reliable. KNOWN LIMITATION: going backward far enough to cross midnight advances the day, same as Set Time - use forward only if that matters."),
    IT_CHEAT("Day Scrub",   CH_MM_DAY_SCRUB,  "CONFIRMED on hardware. Hold {HK} + D-Pad Up/Down to cycle the day 1/2/3, one step per press. Rebind the hold button in Settings. Only takes effect on your next area transition, same as Set Day. KNOWN LIMITATION: going backward is unreliable around that transition - forward is the reliable direction."),
};

// MM3D Battle cheats. Addresses derived from the offline save-file map (SaveGames/, anchored at
// RAM = file_offset + 0x7761D8; see references/) - NOT yet confirmed on hardware.
// USA, v1.1.0 (0004000000125500).
static const Item battleItems[] = {
    IT_CHEAT("Refill Hearts", CH_MM_REFILL_HEARTS, "CONFIRMED on hardware. Fills current health up to your real max (reads capacity from 0x776312, writes it to 0x776314). Applies once."),
    IT_CHEAT("Max Hearts", CH_MM_HEARTS_MAX, "CONFIRMED on hardware. Holds both health capacity and current health at 20 hearts (0x0140) while active - unlocks every heart container. Turning it off restores your real capacity and current health. Address 0x776312/0x776314, u16."),
    IT_CHEAT("Refill Magic", CH_MM_REFILL_MAGIC, "CONFIRMED on hardware. Fills the magic meter to your real cap (0x30 normal, 0x60 if Double Magic is unlocked - reads the flag at 0x77631F). Applies once."),
    IT_CHEAT("Enhanced Defense", CH_MM_DEFENSE, "CONFIRMED on hardware. Halves damage taken while active. Turning it off restores whatever you actually had before. Address 0x776320, u8."),
};

// MM3D Inventory cheats. Max Rupees is CONFIRMED on hardware (Cheat Search: 27 -> 57, poked
// 0x776318, in-game counter matched). The rest are derived from the save-file map and NOT yet
// confirmed. USA, v1.1.0 (0004000000125500).
static const Item inventoryItems[] = {
    IT_CHEAT("Max Rupees (999)", CH_MM_RUPEES_MAX, "CONFIRMED on hardware. Holds your rupee count at 999 while active. Address 0x776318, u16."),
    IT_CHEAT("Fill Rupee Bank (5499)", CH_MM_BANK_FILL, "Not yet confirmed on hardware. Fills your Rupee Bank balance. Applies once. Address 0x777408, u16 = 0x157B. Outside the mapped save block, but the same address style as the confirmed Fishing Hole Pass (no v1.0/v1.1 split), so likely stable."),
    IT_CHEAT("Gilded Sword + Mirror Shield", CH_MM_GILDED_MIRROR, "CONFIRMED on hardware. Grants the Gilded Sword and Mirror Shield. Applies once. Address 0x776352, u8 = 0x23."),
    IT_CHEAT("Razor Sword + Mirror Shield", CH_MM_RAZOR_MIRROR, "Not yet confirmed on hardware. Grants the Razor Sword and Mirror Shield - same address as the Gilded Sword cheat, different tier. Applies once. Address 0x776352, u8 = 0x22."),
    IT_CHEAT("Large Quiver + Big Bomb Bag", CH_MM_QUIVER_BOMBBAG, "CONFIRMED on hardware. Grants a quiver/bomb bag upgrade tier (exact sizes unverified - matches the AR code's own value). Applies once. Address 0x7763CC, u16 = 0x201B."),
    IT_FOLDER("Items (Max/Inf ammo)", F_AMMO),
    IT_FOLDER("Bottles", F_BOTTLES),
    IT_SEP("B BUTTON ITEM"),
    IT_CHEAT("Gilded Sword",      CH_BBUTTON_GILDED,  "Not yet confirmed on hardware. Equips the Gilded Sword on the B button. Applies once. Address 0x77632A, u8 = 0x4F."),
    IT_CHEAT("Great Fairy Sword", CH_BBUTTON_GFSWORD, "Not yet confirmed on hardware. Equips the Great Fairy Sword on the B button. Applies once. Address 0x77632A, u8 = 0x50."),
};

// Bottle contents get their own 2-column grid folder (same layout as Teleport/HOME) - 7 pickers
// with real per-content sprites read a lot better as a grid than crammed into Inventory's list.
static const Item bottlesItems[] = {
    IT_PICKER("Bottle #1", PK_BOTTLE1, "Not yet confirmed on hardware. Sets what Bottle #1 holds. Address 0x776384 is CONFIRMED (already used by Have all Items); the content values are from two independent community AR-code lists that agree, but not hardware-tested here."),
    IT_PICKER("Bottle #2", PK_BOTTLE2, "Not yet confirmed on hardware. Sets what Bottle #2 holds. Address 0x776385 CONFIRMED; content values not individually hardware-tested."),
    IT_PICKER("Bottle #3", PK_BOTTLE3, "Not yet confirmed on hardware. Sets what Bottle #3 holds. Address 0x776386 CONFIRMED; content values not individually hardware-tested."),
    IT_PICKER("Bottle #4", PK_BOTTLE4, "Not yet confirmed on hardware. Sets what Bottle #4 holds. Address 0x776387 CONFIRMED; content values not individually hardware-tested."),
    IT_PICKER("Bottle #5", PK_BOTTLE5, "Not yet confirmed on hardware. Sets what Bottle #5 holds. Address 0x776388 CONFIRMED; content values not individually hardware-tested."),
    IT_PICKER("Bottle #6", PK_BOTTLE6, "Not yet confirmed on hardware. Sets what Bottle #6 holds. Address 0x776389 CONFIRMED; content values not individually hardware-tested."),
    IT_PICKER("Bottle #7", PK_BOTTLE7, "Not yet confirmed on hardware. Sets what Bottle #7 holds. Address 0x77638A CONFIRMED; content values not individually hardware-tested."),
};

// MM3D ammo max/inf toggles. Addresses and per-slot values are from the AR code list
// (references/mm3d-ar-cheats-usa-0004000000125500.txt) - the offline save-file map could not
// independently confirm which slot is which, so these are NOT yet confirmed on hardware.
static const Item ammoItems[] = {
    IT_CHEAT("Max/Inf Arrows",      CH_MM_AMMO_ARROWS, "Not yet confirmed on hardware. Holds your arrow count at 99 while active. Address 0x776391, u8."),
    IT_CHEAT("Max/Inf Bombs",       CH_MM_AMMO_BOMBS,  "Not yet confirmed on hardware. Holds your bomb count at 99 while active. Address 0x776396, u8."),
    IT_CHEAT("Max/Inf Bombchus",    CH_MM_AMMO_CHUS,   "Not yet confirmed on hardware. Holds your Bombchu count at 99 while active. Address 0x776397, u8."),
    IT_CHEAT("Max/Inf Deku Sticks", CH_MM_AMMO_STICKS, "Not yet confirmed on hardware. Holds your Deku Stick count at 99 while active. Address 0x776398, u8."),
    IT_CHEAT("Max/Inf Deku Nuts",   CH_MM_AMMO_NUTS,   "Not yet confirmed on hardware. Holds your Deku Nut count at 50 while active. Address 0x776399, u8."),
    IT_CHEAT("Max/Inf Magic Beans", CH_MM_AMMO_BEANS,  "Not yet confirmed on hardware. Holds your Magic Bean count at 99 while active. Address 0x77639A, u8."),
    IT_CHEAT("Max/Inf Powder Keg",  CH_MM_AMMO_KEG,    "Not yet confirmed on hardware. Holds your Powder Keg count at 99 while active. Address 0x77639C, u8."),
};

// MM3D Quest cheats. All decoded from the AR code list's conditional/loop opcodes and
// cross-checked against the save-file map (see references/ + CTRComposer-Repo-Kickoff.md).
// CONFIRMED-status per cheat noted below; not yet exercised on hardware.
static const Item questItems[] = {
    IT_CHEAT("Have all Items", CH_MM_ALL_ITEMS, "Not yet confirmed on hardware. Fills the 16 main item slots (0x776355-0x776364), byte-for-byte matching a real 100%-save item array. Applies once."),
    IT_CHEAT("Have all Masks", CH_MM_ALL_MASKS, "Not yet confirmed on hardware. Fills all 24 mask slots (0x77636C-0x776383) with the acquisition-order id sequence 0x32-0x49, matching a real 100% save exactly. Applies once."),
    IT_CHEAT("All Bosses and Songs", CH_MM_ALL_BOSSES_SONGS, "Not yet confirmed on hardware. Sets the boss/song bitfields to the values read from a real 100% save (0xCF 0xF7 0xCF) - NOT the AR code's own 0xFF 0xFF 0xFF, which doesn't match any legitimate save. Address 0x7763D0-0x7763D2. Applies once."),
    IT_CHEAT("All Stray Fairies", CH_MM_ALL_FAIRIES, "Not yet confirmed on hardware. Fills all four dungeon stray-fairy bytes (0x7763E8-0x7763EB) - the AR code's own version only fills one. Applies once."),
    IT_CHEAT("Fishing Hole Pass", CH_TEST_FISHING, "CONFIRMED on hardware. Grants the Fishing Hole Pass (lets you borrow a fishing rod for free at either Fishing Hole). Applies once. Address 0x7776C0, u8 = 0x63."),
};

// MM3D Misc. Moon Jump was the first base+offset cheat in this plugin - the pointer chain
// (0x08363784 -> +0x4AC -> player actor) came from the AR code's own decode, never
// independently re-derived, and it's now CONFIRMED working on hardware.
// FORMS ("PLAY AS" field, 0x7761FE): CONFIRMED on hardware the change only takes effect on your
// NEXT area transition (door, warp, load), not instantly. Goron and Deku Link were tried and
// don't hold (the mask comes off on its own moments after loading), so only these 3 values are
// offered. RECOMMENDED: pick Normal Link and change areas before switching to a different form,
// to avoid state left over from the previous one (chaining straight off Fierce Deity into
// another forced form was seen to break control). Zora and Fierce Deity use their real mask
// icons (The Spriters Resource, Colbydude's Item Icons sheet); Normal Link has no mask to show,
// so it keeps the hand-drawn placeholder.
static const Item miscItems[] = {
    IT_CHEAT("Moon Jump", CH_MM_MOONJUMP, "CONFIRMED on hardware. Hold {L}+{A} to rise into the air, release to fall. Mind the fall distance."),
    IT_SEP("FORMS"),
    IT_CHEAT("Normal Link",  CH_PLAY_NORMAL,      "CONFIRMED on hardware. Applies once, on your next area transition. Address 0x7761FE, u8 = 0x04."),
    IT_CHEAT("Zora",         CH_PLAY_ZORA,        "CONFIRMED on hardware. Applies once, on your next area transition. Address 0x7761FE, u8 = 0x02."),
    IT_CHEAT("Fierce Deity", CH_PLAY_FIERCEDEITY, "CONFIRMED on hardware. Bypasses the vanilla boss-arena-only restriction entirely - fully controllable. Applies once, on your next area transition. Address 0x7761FE, u8 = 0x00."),
};

// Teleport folder: one row per warps[] entry (label/desc pulled from warps[] at draw time via
// the index in IT_WARP), same architecture as OcarinaCTRComposer's Teleport. Section headers
// are cosmetic grouping only; the Filter row's All/Overworld/Dungeons/Owl Statues split is driven
// by each warp's `isDungeon`/`isOwl` flags, not by which section a row visually sits in.
static const Item teleportItems[] = {
    IT_WARP_WIDE(NULL, 0, NULL),   // Reload current scene (full-width, first)
    IT_TPFILTER,                   // category filter (All / Overworld / Dungeons / Owl Statues)
    IT_SEP("CLOCK TOWN"),
    IT_WARP(NULL, 1, NULL), IT_WARP(NULL, 2, NULL), IT_WARP(NULL, 3, NULL),
    IT_WARP(NULL, 4, NULL), IT_WARP(NULL, 5, NULL),
    IT_SEP("SWAMP"),
    IT_WARP(NULL, 6, NULL), IT_WARP(NULL, 7, NULL), IT_WARP(NULL, 8, NULL), IT_WARP(NULL, 9, NULL),
    IT_SEP("MOUNTAIN"),
    IT_WARP(NULL, 10, NULL), IT_WARP(NULL, 11, NULL), IT_WARP(NULL, 12, NULL),
    IT_WARP(NULL, 13, NULL), IT_WARP(NULL, 14, NULL), IT_WARP(NULL, 15, NULL),
    IT_SEP("GREAT BAY"),
    IT_WARP(NULL, 16, NULL), IT_WARP(NULL, 17, NULL), IT_WARP(NULL, 18, NULL), IT_WARP(NULL, 19, NULL),
    IT_SEP("IKANA"),
    IT_WARP(NULL, 20, NULL), IT_WARP(NULL, 21, NULL), IT_WARP(NULL, 22, NULL),
    IT_SEP("RANCH / MOON"),
    IT_WARP(NULL, 23, NULL), IT_WARP(NULL, 24, NULL), IT_WARP(NULL, 25, NULL),
};
#endif // !TOOLS_ONLY

static const Item settingsItems[] = {
    IT_SEP("GENERAL"),
#if TOOLS_ONLY
    IT_CHEAT("Change Theme", CH_CFG_THEME, "Recolor every menu live. Your pick is saved to the SD card."),
    IT_CHEAT("Language", CH_CFG_LANG, "Press {A} to cycle the menu language. Translations load from the plugin folder, under lang/. English is built in."),
#else
    IT_CHEAT("Change Theme", CH_CFG_THEME, "Recolor every menu live. The template ships one neutral theme; add your own to THEMES[] in Includes/themes.h. Your pick is saved."),
    IT_CHEAT("Language", CH_CFG_LANG, "Press {A} to cycle the menu language. Translations load from <plugin folder>/lang/. The template ships English only."),
#endif
    IT_CHEAT("Toggle notifications (toast)", CH_CFG_TOAST, "Shows a small notification in-game when something is toggled."),
#if !TOOLS_ONLY
    IT_CHEAT("Auto-fill Tracker on open", CH_CFG_AUTOFILL, "When on, the Tracker syncs itself from game memory every time you open it."),
#endif
    IT_SEP("IN-GAME HOTKEYS"),
    IT_CHEAT("Quick Menu hotkey", CH_CFG_QMKEY, "Press {A} to cycle the button combo that opens the quick menu in game."),
#if !TOOLS_ONLY
    IT_CHEAT("Time Scrub hotkey", CH_CFG_HK1, "Press {A} to cycle the button held for Time Scrub (Time folder). Default {R}."),
    IT_CHEAT("Day Scrub hotkey",  CH_CFG_HK2, "Press {A} to cycle the button held for Day Scrub (Time folder). Default {L}."),
#endif
    IT_CHEAT("Reset hotkeys to default", CH_CFG_HKRESET, "Press {A} to restore the Quick Menu and example hotkeys to their defaults ({L}+SELECT / {Y} / {X})."),
};

#define FCOUNT(a) (int)(sizeof(a) / sizeof((a)[0]))
static const Folder folders[NUM_FOLDERS] = {
#if TOOLS_ONLY
    { "CTRComposer Tools",    rootItems,     FCOUNT(rootItems) },
    { "Settings",             settingsItems, FCOUNT(settingsItems) },
#else
    { "MajoraCTRComposer",    rootItems,     FCOUNT(rootItems) },
    { "Time",                 timeItems,     FCOUNT(timeItems) },
    { "Battle",               battleItems,   FCOUNT(battleItems) },
    { "Inventory",            inventoryItems, FCOUNT(inventoryItems) },
    { "Items",                ammoItems,     FCOUNT(ammoItems) },
    { "Bottles",              bottlesItems,  FCOUNT(bottlesItems) },
    { "Quest",                questItems,    FCOUNT(questItems) },
    { "Misc.",                miscItems,     FCOUNT(miscItems) },
    { "Teleport",             teleportItems, FCOUNT(teleportItems) },
    { "Examples",             exampleItems,  FCOUNT(exampleItems) },
    { "Tools",                toolsItems,    FCOUNT(toolsItems) },
    { "Settings",             settingsItems, FCOUNT(settingsItems) },
#endif
};
