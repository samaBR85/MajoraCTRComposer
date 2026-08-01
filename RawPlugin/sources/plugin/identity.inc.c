// Public release version + build counter. Bump the build number EVERY build: the on-screen tag
// is your confirmation that the .3gx actually on the SD card is the one you just compiled.
// Change it with a real edit and check it on screen - a blind sed can silently no-op and leave
// you debugging a stale binary.
// TOOLS-ONLY BUILD, for use as Luma's universal plugin: /luma/plugins/default.3gx
//
// Luma falls back to that file for ANY title with no plugin folder of its own, so one build
// follows you into every game. Cheats cannot work that way - an address is specific to one game
// and one region - but the memory TOOLS are completely game-agnostic, and those are what is
// worth having everywhere.
//
// Set to 1 and the menu drops the per-game furniture (the example cheats, the tracker, the game
// guide) and surfaces Cheat Search / RAM Dumper / Hex Editor directly on HOME. Nothing else
// about the engine changes.
//
// Note that as default.3gx the plugin loads into EVERYTHING - the Home Menu, applets, homebrew -
// not just games, and it flips the host process to RWX and pauses its threads like it does
// anywhere else. That is a much broader blast radius than a single title. Treat it as
// experimental.
#define TOOLS_ONLY 0

// Opt-in: respond to Luma's process-exit event and tear the plugin down before the game dies.
//
// OFF because it was MEASURED not to work - see the block in ThreadMain. Left in the tree
// because it costs nothing and a future Luma build may start delivering the event; flipping
// this to 1 also writes a marker file at shutdown so you can tell in one run.
#define EXIT_HANDSHAKE 0

#define PLUGIN_VER "v0.1.0 build 48"   // full string - About screen and pause box (have room)

// Name and short tag follow the build flavour automatically, so flipping TOOLS_ONLY is the ONLY
// edit needed to produce the other binary. Deriving these beat setting them by hand: the local
// build and CI had already drifted apart doing it manually.
#if TOOLS_ONLY
#define PLUGIN_NAME "CTRComposer Tools"
#define PLUGIN_TAG  "T1.0"              // compact tag - cramped menu title bar
#else
#define PLUGIN_NAME "MajoraCTRComposer"
#define PLUGIN_TAG  "b48"
#endif
