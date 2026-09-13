// ===================== Cheat IDs =====================
// THIS IS THE GAME-SPECIFIC PART. Everything else in this file is reusable engine.
//
// Add one enum entry per cheat, then:
//   - give it a row in a Folder below (IT_CHEAT),
//   - implement it in ApplyCheats() (continuous) or OneShot() (applied once).
// The CH_CFG_* entries at the end are not cheats - they are Settings rows that
// reuse the same row-drawing code, which is why they live in the same enum.
enum {
    // ---- EXAMPLE cheats: replace these with your game's ----
    CH_EX_DIRECT,     // continuous: direct u16 write to a fixed address
    CH_EX_BYTE,       // continuous: u8 write
    CH_EX_WORD,       // continuous: u32 write
    CH_EX_BASEOFF,    // continuous: base pointer + offset write
    CH_EX_HOTKEY,     // continuous: only while a rebindable hotkey is held
    CH_EX_ONESHOT,    // one-shot: applied once, when you select it
    CH_EX_ONESHOT2,   // one-shot with a custom result message
    // ---- MM3D: Time (v1.1 USA, 0004000000125500) ----
    CH_MM_DAY1, CH_MM_DAY2, CH_MM_DAY3,
    CH_MM_TIME_6AM, CH_MM_TIME_10AM, CH_MM_TIME_6PM,
    CH_MM_TIME_SCRUB, CH_MM_DAY_SCRUB, CH_MM_ISOT_THIRD,
    // ---- MM3D: Battle ----
    CH_MM_REFILL_HEARTS, CH_MM_HEARTS_MAX, CH_MM_REFILL_MAGIC,
    // ---- MM3D: Inventory ----
    CH_MM_RUPEES_MAX, CH_MM_DEFENSE, CH_MM_GILDED_MIRROR, CH_MM_QUIVER_BOMBBAG,
    CH_BBUTTON_GILDED, CH_BBUTTON_GFSWORD, CH_MM_RAZOR_MIRROR, CH_MM_BANK_FILL,
    // ---- MM3D: Items (ammo max/inf, continuous) ----
    CH_MM_AMMO_ARROWS, CH_MM_AMMO_BOMBS, CH_MM_AMMO_CHUS, CH_MM_AMMO_STICKS,
    CH_MM_AMMO_NUTS, CH_MM_AMMO_BEANS, CH_MM_AMMO_KEG,
    // ---- MM3D: Quest ----
    CH_MM_ALL_ITEMS, CH_MM_ALL_MASKS, CH_MM_ALL_BOSSES_SONGS, CH_MM_ALL_FAIRIES, CH_TEST_FISHING,
    // ---- MM3D: Misc ----
    CH_MM_MOONJUMP,
    // ---- MM3D: Play as... (CONFIRMED - see the folder comment for what's safe and why) ----
    CH_PLAY_NORMAL, CH_PLAY_ZORA, CH_PLAY_FIERCEDEITY,
    // ---- MM3D: Minigames. CODE PATCHES - instruction rewrites in .text, applied on the toggle
    // edge and reverted to the captured original when turned off. Never persisted (a code patch
    // must not survive a reboot with the game already past that code). See MG_PATCHES.
    CH_MG_TOWN_GALLERY, CH_MG_SWAMP_GALLERY, CH_MG_BEAVER, CH_MG_BOAT_JUMP, CH_MG_HONEY_DARLING,
    // ---- MM3D: Movement restores. Also .text/param patches (supervisor write); originals are
    // captured at runtime on enable and put back on disable (we have no recorded originals). ----
    CH_MV_DEKU_WALK, CH_MV_ZORA_SWIM,
    // ---- Settings rows (not cheats) ----
    CH_CFG_TOAST, CH_CFG_AUTOFILL, CH_CFG_QMKEY, CH_CFG_HK1, CH_CFG_HK2,
    CH_CFG_HKRESET, CH_CFG_THEME, CH_CFG_LANG,
    NUM_CHEATS
};
static u8 cheatState[NUM_CHEATS];
static u8 favorite[NUM_CHEATS];

// Brief green-check flash so instant (one-shot) cheats give in-menu feedback
static int flashCheat = -1;
static int flashTicks = 0;
static const char *flashMsg = "OK";     // shown next to the cheat during the flash
static const char *g_oneShotMsg = "OK"; // OneShot() sets this: "OK" / a custom result string

static int configDirty = 0; // settings changed -> save config on menu close
static int favDirty = 0;    // a favorite toggled -> save Favorites.txt on menu close
static int g_themeIdx = 0, g_themeParchment = 0; // active theme (colors live in CGOLD/... below)

// ===================== Where this plugin keeps its files =====================
// Luma loads a plugin from  sdmc:/luma/plugins/<TitleID>/<Name>.3gx  and this is where the
// plugin keeps Settings.cfg, Favorites.txt, Tracker.txt, lang/, guide/ and dumps/.
//
// >>> SET THIS to your game's folder once you know its Title ID. <<<
//
//     #define PLUGIN_DIR "/luma/plugins/0004000000033500/"
//
// Left empty, everything lands in /luma/plugins/ itself. That WORKS, and it is fine for a
// first run before you know the Title ID - but the folder is shared by every game, so two
// plugins built from this template would fight over the same Settings.cfg. Do not ship it
// that way.
//
// (An earlier version tried to discover this at runtime from PluginHeader.pluginPathPA.
// That field is a PHYSICAL address, and the PA_PTR mirror it needs is only valid for the IO
// region a plugin gets mapped - like the HID register - not for arbitrary FCRAM. On hardware
// the read never produced a usable path and it silently fell back here, which is how config
// ended up in /luma/plugins/. A compile-time constant is predictable; that beats clever.)
#define PLUGIN_DIR "/luma/plugins/0004000000125500/"

#define DEFAULT_PLUGIN_DIR "/luma/plugins/"

// Build "<plugin dir><leaf>" into a rotating static buffer. Two buffers so a caller
// can hold two paths at once (e.g. read one file while naming another).
static const char *PlgPath(const char *leaf)
{
    static char buf[2][320];
    static int which = 0;
    const char *dir = (PLUGIN_DIR[0] != 0) ? PLUGIN_DIR : DEFAULT_PLUGIN_DIR;
    char *out = buf[which]; which ^= 1;
    int n = 0;
    for (const char *s = dir;  *s && n < 250; ++s) out[n++] = *s;
    for (const char *s = leaf; *s && n < 315; ++s) out[n++] = *s;
    out[n] = 0;
    return out;
}
