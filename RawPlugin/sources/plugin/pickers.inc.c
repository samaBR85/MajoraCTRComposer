// ===================== Pickers (choose a value from a list) =====================
// A picker is a menu row that opens a list and writes the chosen value to one address.
// Good for "which item is in this slot" style cheats where a toggle makes no sense.
// `icon` is a sprite key (see MSPR_* above) for the picker screen and the row's live-value icon;
// -1 means "no sheet art for this one" and the picker falls back to a plain color swatch.
typedef struct { const char *name; u8 val; int icon; } PickOpt;
typedef struct { const char *title; const PickOpt *opts; int count; u32 addr; } Picker;

// Bottle contents - the address for each of the 7 slots is CONFIRMED on hardware (already
// used by Have all Items/Refill Magic-adjacent cheats). Values are the real ItemId enum from
// the zeldaret/mm N64 decompilation (include/z64item.h), not the earlier community AR list -
// that list had several bottle entries silently off by one or two starting at "Big Poe"
// (0x1C is actually Blue Fire; Big Poe is 0x1E; the AR list's "Mystery/Mouldy Milk" at 0x26/0x27
// are really Hylian Loach and Obaba's Drink). Item IDs match 1:1 with our own confirmed B-Button
// value for Fierce Deity's Mask (0x35, in the same enum), so this table is trusted over the AR
// list. Writing a non-"Empty" value has not been individually hardware-tested here.
static const PickOpt bottleOpts[] = {
    { "Empty",            0x12, MSPR_BOTTLE },
    { "Red Potion",       0x13, MSPR_B_REDPOTION },
    { "Green Potion",     0x14, MSPR_B_GREENPOTION },
    { "Blue Potion",      0x15, MSPR_B_BLUEPOTION },
    { "Fairy",            0x16, MSPR_B_FAIRY },
    { "Deku Princess",    0x17, MSPR_B_DEKUPRINCESS },
    { "Milk",             0x18, MSPR_B_MILK },
    { "Milk (Half)",      0x19, MSPR_B_MYSTMILK },
    { "Fish",             0x1A, MSPR_B_FISH },
    { "Bug",              0x1B, MSPR_B_BUG },
    { "Blue Fire",        0x1C, MSPR_B_HOTSPRING },   // playful stand-in: no distinct art on the sheet
    { "Poe",              0x1D, MSPR_B_BIGPOE },      // playful stand-in: shares the sheet's ghostly glow bottle
    { "Big Poe",          0x1E, MSPR_B_BIGPOE },      // playful stand-in: same as Poe, no distinct art
    { "Spring Water",     0x1F, MSPR_B_SPRINGWATER },
    { "Hot Spring Water", 0x20, MSPR_B_HOTSPRING },
    { "Zora Egg",         0x21, MSPR_B_ZORAEGG },
    { "Gold Dust",        0x22, MSPR_B_GOLDDUST },
    { "Magic Mushroom",   0x23, MSPR_B_MUSHROOM },
    { "Sea Horse",        0x24, MSPR_B_SEAHORSE },
    { "Chateau Romani",   0x25, MSPR_B_CHATEAU },
    { "Hylian Loach",     0x26, MSPR_B_FISH },        // playful stand-in: it's a fish too, no distinct art
    { "Obaba's Drink",    0x27, MSPR_B_CHATEAU },     // playful stand-in: another "bottled drink" icon
};
#define NUM_BOTTLE_OPTS (int)(sizeof(bottleOpts)/sizeof(bottleOpts[0]))

enum { PK_BOTTLE1, PK_BOTTLE2, PK_BOTTLE3, PK_BOTTLE4, PK_BOTTLE5, PK_BOTTLE6, PK_BOTTLE7, NUM_PICKERS };
static const Picker pickers[NUM_PICKERS] = {
    { "Bottle #1",   bottleOpts,  NUM_BOTTLE_OPTS, 0x776384 },
    { "Bottle #2",   bottleOpts,  NUM_BOTTLE_OPTS, 0x776385 },
    { "Bottle #3",   bottleOpts,  NUM_BOTTLE_OPTS, 0x776386 },
    { "Bottle #4",   bottleOpts,  NUM_BOTTLE_OPTS, 0x776387 },
    { "Bottle #5",   bottleOpts,  NUM_BOTTLE_OPTS, 0x776388 },
    { "Bottle #6",   bottleOpts,  NUM_BOTTLE_OPTS, 0x776389 },
    { "Bottle #7",   bottleOpts,  NUM_BOTTLE_OPTS, 0x77638A },
};

// A picker points at a GAME address, and that address is a placeholder (0) until you fill it
// in - and even then it can be wrong, or unmapped in the current scene. NEVER dereference it
// blind: an unmapped read on the 3DS is a data abort that hard-freezes the console, with the
// menu still on screen. These two wrap every picker access.
static int PickerRead(const Picker *pk, u8 *out)
{
    if (!pk->addr || !MemReadable(pk->addr)) return 0;
    *out = R8(pk->addr);
    return 1;
}
static int PickerWrite(const Picker *pk, u8 v)
{
    if (!pk->addr || !MemWritable(pk->addr)) return 0;
    W8(pk->addr, v);
    return 1;
}

// Mapped teleport destinations. Entrance indices are MM3D v1.1.0 USA values, sourced from
// PhlexPlexico/mm3d-practice-tools' source/msys/include/entrances.h. The Teleport mechanism
// itself is CONFIRMED on hardware (see MM_Warp()); Termina Field (index 4) was the exact entry
// individually hardware-tested - the rest reuse the same confirmed recipe, not each tested.
// isOwl: one of MM3D's 10 real Owl Statue warp points (user-supplied list, cross-checked against
// this table). Mountain Village and Goron Village each get TWO rows here (Spring/Winter scene
// variants of the same physical statue), so 12 rows carry isOwl=1 for 10 real statues. Snowhead's
// description used to claim "(from owl statue)" too - that was wrong; the real list does not
// include it, so its owl flag stays 0 and the description was corrected.
typedef struct { const char *name; u16 entrance; u8 isDungeon; u8 isOwl; const char *desc; } Warp;
static const Warp warps[] = {
    /* 0 */  { "Reload current scene",   0xFFFF, 0, 0, "Reloads the area you're in. The safest warp - use it first to confirm teleport works on your game." },
    // --- Clock Town (1..5) ---
    /* 1 */  { "South Clock Town", 0xD890, 0, 1, "Warp to South Clock Town (from owl statue)." },
    /* 2 */  { "East Clock Town",  0xD200, 0, 0, "Warp to East Clock Town (from Termina Field)." },
    /* 3 */  { "West Clock Town",  0xD400, 0, 0, "Warp to West Clock Town (from Termina Field)." },
    /* 4 */  { "North Clock Town", 0xD600, 0, 0, "Warp to North Clock Town (from Termina Field)." },
    /* 5 */  { "Termina Field",    0x5460, 0, 0, "CONFIRMED on hardware. Warp to Termina Field (from South Clock Town)." },
    // --- Swamp (6..9) ---
    /* 6 */  { "Southern Swamp",  0x0CA0, 0, 1, "Warp to Southern Swamp (from owl statue)." },
    /* 7 */  { "Deku Palace",     0x5000, 0, 0, "Warp to Deku Palace (front doorway)." },
    /* 8 */  { "Woodfall",        0x8640, 0, 1, "Warp to Woodfall (from owl statue)." },
    /* 9 */  { "Woodfall Temple", 0x3000, 1, 0, "Warp inside Woodfall Temple (front room)." },
    // --- Mountain (10..15) ---
    /* 10 */ { "Mountain Village (Spring)", 0xAE80, 0, 1, "Warp to Mountain Village, Spring (from owl statue)." },
    /* 11 */ { "Mountain Village (Winter)", 0x9A80, 0, 1, "Warp to Mountain Village, Winter (from owl statue)." },
    /* 12 */ { "Goron Village (Spring)",    0x8A00, 0, 1, "Warp to Goron Village, Spring (from owl statue)." },
    /* 13 */ { "Goron Village (Winter)",    0x9400, 0, 1, "Warp to Goron Village, Winter (from owl statue)." },
    /* 14 */ { "Snowhead",        0xB230, 0, 0, "Warp to Snowhead." },
    /* 15 */ { "Snowhead Temple", 0x3C00, 1, 0, "Warp inside Snowhead Temple." },
    // --- Great Bay (16..19) ---
    /* 16 */ { "Great Bay Coast",  0x68B0, 0, 1, "Warp to Great Bay Coast (from owl statue)." },
    /* 17 */ { "Zora Cape",        0x6A60, 0, 1, "Warp to Zora Cape (from owl statue)." },
    /* 18 */ { "Zora Hall",        0x6000, 0, 0, "Warp to Zora Hall (atrium)." },
    /* 19 */ { "Great Bay Temple", 0x8C00, 1, 0, "Warp inside Great Bay Temple." },
    // --- Ikana (20..22) ---
    /* 20 */ { "Ikana Canyon",       0x2040, 0, 1, "Warp to Ikana Canyon (from owl statue)." },
    /* 21 */ { "Stone Tower",        0xAA30, 0, 1, "Warp to Stone Tower (from owl statue)." },
    /* 22 */ { "Stone Tower Temple", 0x2600, 1, 0, "Warp inside Stone Tower Temple." },
    // --- Ranch / Moon (23..25) ---
    /* 23 */ { "Romani Ranch", 0x6400, 0, 0, "Warp to Romani Ranch (from Milk Road)." },
    /* 24 */ { "Milk Road",    0x3E40, 0, 1, "Warp to Milk Road (from owl statue)." },
    /* 25 */ { "The Moon",     0xC800, 0, 0, "Warp to The Moon (from Clock Tower rooftop)." },
};
#define NUM_WARPS (int)(sizeof(warps)/sizeof(warps[0]))
