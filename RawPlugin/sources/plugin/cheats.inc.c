// ===================== Cheat implementations =====================
//
//   >>> THIS IS THE ONLY SECTION THAT IS GENUINELY GAME-SPECIFIC. <<<
//
// The plugin runs INSIDE the game's process, so writing game memory is just a pointer
// write - W8/W16/W32 above. All you need are the addresses. Get them from an existing
// source (a .plg, an Action Replay code bank, a community plugin - each write becomes
// one line of C) or discover them with the engine's own Cheat Search tool.
//
// Addresses are ALWAYS region- and version-specific. Re-anchor them when you change
// region, or the writes land somewhere random and crash the game.
//
// The four EXAMPLE_* addresses below are deliberately fake. They are guarded so the
// template is safe to run as-is: EXAMPLE_ENABLED is 0, so nothing is ever written.
// Set it to 1 once you have replaced the addresses with real ones.
#define EXAMPLE_ENABLED 0

// EXAMPLE - replace with your game's address. A counter you want pinned to a value.
#define EXAMPLE_ADDR_DIRECT   0x00000000u
#define EXAMPLE_VALUE_DIRECT  0x03E7       // 999

// EXAMPLE - replace with your game's address. The classic base+offset pattern: a
// pointer to the player/entity struct, and a field at a fixed offset inside it.
#define EXAMPLE_ADDR_BASE     0x00000000u  // holds a pointer
#define EXAMPLE_OFF_FIELD     0x00u
#define EXAMPLE_VALUE_FIELD   0x00000000u

// EXAMPLE - replace with your game's address. Written once, when selected in the menu.
#define EXAMPLE_ADDR_ONESHOT  0x00000000u
#define EXAMPLE_VALUE_ONESHOT 0xFF

// Read the base pointer for base+offset cheats. Returns 0 when it looks unusable, so
// every caller can just check for 0 and skip - never write through a null base.
static u32 ExampleBase(void)
{
    if (!EXAMPLE_ENABLED) return 0;
    return R32(EXAMPLE_ADDR_BASE);
}

// Teleport - CONFIRMED on hardware (MM3D USA v1.1.0). GCTX_BASE_PTR (0x08363784) is the same
// stable pointer Moon Jump uses; GlobalContext itself is heap-allocated and its address changes
// every session, so it must be read fresh every call, never cached. `entrance` is one of MM3D's
// ~120 real entrance indices (see references/ - table sourced from PhlexPlexico/mm3d-practice-
// tools' source/msys/include/entrances.h). Returns 0 (does nothing) if GCTX isn't readable.
static int MM_Warp(u16 entrance)
{
    u32 gctx = R32(0x08363784);
    if (!gctx) return 0;
    // entrance==0xFFFF = "reload current scene": read CommonDataSub1.entrance (0x7796F0 =
    // CommonData_base 0x7761D8 + 0x3518, the confirmed CommonData_base + an UNCONFIRMED
    // sub-offset from common_data.h). Mirrors OoT's SAVE_ENTRANCE reload pattern.
    if (entrance == 0xFFFF) entrance = (u16)R32(0x7796F0);
    W32(0x7761F4, 0);             // SaveData.cutscene_stuff = 0 (clear pending cutscene)
    W16(gctx + 0xC52E, entrance); // GCTX->next_entrance
    W8(gctx + 0xC530, 3);         // GCTX transition-type byte
    W16(0x00789910, entrance);    // CommonData.sub13s[0].entrance_index (respawn mirror)
    W8(gctx + 0xC529, 0x14);      // GCTX->field_C529 (trigger - writing 0x14 starts the load)
    return 1;
}

// One-shot cheats: applied instantly when selected in the menu, then the row flashes.
// Return 1 if `id` is a one-shot (so the menu knows not to treat it as a toggle).
// Set g_oneShotMsg to override the "OK" flash text - handy for ADDED/REMOVED toggles.
static int OneShot(int id)
{
    g_oneShotMsg = "OK"; // default flash text
    switch (id)
    {
        case CH_EX_ONESHOT:
            if (EXAMPLE_ENABLED) W8(EXAMPLE_ADDR_ONESHOT, EXAMPLE_VALUE_ONESHOT);
            else g_oneShotMsg = "EXAMPLE";  // nothing written: see EXAMPLE_ENABLED above
            return 1;

        // Same thing, but reporting a RESULT. A toggle-style one-shot (flip a bit, then read
        // it back) can say which way it went, so the flash is unambiguous instead of a bare OK.
        case CH_EX_ONESHOT2:
            if (EXAMPLE_ENABLED)
            {
                u8 v = (u8)(R8(EXAMPLE_ADDR_ONESHOT) ^ 0x01);
                W8(EXAMPLE_ADDR_ONESHOT, v);
                g_oneShotMsg = (v & 0x01) ? "ADDED" : "REMOVED";
            }
            else g_oneShotMsg = "EXAMPLE";
            return 1;

        // MM3D (USA v1.1.0, 0004000000125500). Day / time-of-day: single-byte writes, the
        // safest possible cheats. See timeItems[] above for the address/evidence notes.
        case CH_MM_DAY1: W8(0x7761EC, 0x01); return 1;
        case CH_MM_DAY2: W8(0x7761EC, 0x02); return 1;
        case CH_MM_DAY3: W8(0x7761EC, 0x03); return 1;
        // CONFIRMED on hardware: writing a LOWER raw time than the current one makes the game
        // advance the day on its own (it reads the decrease as midnight passing). Re-asserting
        // 0x7761EC right after the write was tried and does NOT stop it - whatever the game
        // actually compares isn't this byte. Left as a known limitation; not chased further.
        case CH_MM_TIME_6AM:  W8(0x7761F9, 0x40); return 1;
        case CH_MM_TIME_10AM: W8(0x7761F9, 0x6B); return 1;
        case CH_MM_TIME_6PM:  W8(0x7761F9, 0xC0); return 1;

        // MM3D Battle/Inventory one-shots - derived from the save-file map, CONFIRMED on hardware
        // (see battleItems[]/inventoryItems[] above for evidence per cheat).
        case CH_MM_REFILL_HEARTS: W16(0x776314, R16(0x776312)); return 1;
        case CH_MM_REFILL_MAGIC:  W8(0x776317, R8(0x77631F) ? 0x60 : 0x30); return 1;
        case CH_MM_GILDED_MIRROR: W8(0x776352, 0x23); return 1;
        case CH_MM_RAZOR_MIRROR:  W8(0x776352, 0x22); return 1; // same address, the non-gilded upgrade tier
        case CH_MM_QUIVER_BOMBBAG: W16(0x7763CC, 0x201B); return 1;

        // Rupee bank. CONFIRMED on hardware. Address is outside the mapped 0x776xxx save block,
        // in the same "0x777xxx, no v1.0/v1.1 split" cluster as Fishing Hole Pass.
        case CH_MM_BANK_FILL: W16(0x777408, 0x157B); return 1; // 5499, the AR code's own max

        // B Button item (0x77632A). CONFIRMED on hardware. Fierce Deity Mask is deliberately not
        // offered here - it duplicates Misc's "Play as..." Fierce Deity row.
        case CH_BBUTTON_GILDED:  W8(0x77632A, 0x4F); return 1;
        case CH_BBUTTON_GFSWORD: W8(0x77632A, 0x50); return 1;

        // MM3D Quest one-shots - decoded from the AR list's loop/conditional opcodes,
        // cross-checked against a real 100% save. CONFIRMED on hardware.
        case CH_MM_ALL_ITEMS:
        {
            static const u8 buf[16] = {
                0x01,0x02,0x03,0x04,0xFF,0x06,0x07,0x08,0x09,0x0A,0xFF,0x0C,0x0D,0x0E,0x0F,0x10
            };
            memcpy((void *)0x776355, buf, sizeof(buf));
            return 1;
        }
        case CH_MM_ALL_MASKS:
            for (int i = 0; i < 24; ++i) W8(0x77636C + i, (u8)(0x32 + i));
            return 1;
        case CH_MM_ALL_BOSSES_SONGS:
            W8(0x7763D0, 0xCF); W8(0x7763D1, 0xF7); W8(0x7763D2, 0xCF);
            return 1;
        case CH_MM_ALL_FAIRIES:
            for (int i = 0; i < 4; ++i) W8(0x7763E8 + i, 0x0F);
            return 1;

        // Teleport activation moved to the dedicated warp handler at the BUTTON_A site (it
        // needs to close the menu after warping, like OoT's Teleport does) - see MM_Warp()
        // and the `it->warp >= 0` case near the other BUTTON_A handling.

        // "PLAY AS" field (0x7761FE). CONFIRMED on hardware the change only takes effect on
        // your NEXT area transition (door, warp, load), never instantly. Only these 3 values
        // are exposed - see the Play as... folder comment (miscItems) for what was tried and
        // ruled out (Goron/Deku Link don't hold; chaining straight off Fierce Deity into
        // another forced form breaks control).
        case CH_PLAY_NORMAL:      W8(0x7761FE, 0x04); return 1;
        case CH_PLAY_ZORA:        W8(0x7761FE, 0x02); return 1;
        case CH_PLAY_FIERCEDEITY: W8(0x7761FE, 0x00); return 1;

        // Fishing Hole Pass. CONFIRMED on hardware - address from a community AR code list
        // (GBAtemp, gamegenie.53lu.com), which labeled it "Fishing Tickets"; the in-game item
        // it actually grants is the Fishing Hole Pass (free rod loan), not a ticket count.
        case CH_TEST_FISHING: W8(0x7776C0, 0x63); return 1;

        // Add your one-shots here:
        //   case CH_MY_CHEAT: W16(0x00123456, 0x0064); return 1;
        //
        // For a CODE patch (an instruction rewrite in the read-only .text segment):
        //   svcControlProcess(CUR_PROCESS_HANDLE, PROCESSOP_SET_MMU_TO_RWX, 0, 0); // once
        //   ALWAYS save the original instruction first so the cheat can be switched off,
        //   then W32() the new one and flush:
        //   svcFlushEntireDataCache(); svcInvalidateEntireInstructionCache();
        // NEVER auto-enable a code patch on boot.
    }
    return 0;
}

#include "../engine/input.inc.c"

// MM3D minigame CODE PATCHES. Each row rewrites one 4-byte ARM instruction in the game's .text
// (the process is already RWX - see main.c). We only act on the toggle edge, verify the current
// word against the original before touching it, and restore that original when turned off - so an
// address that does not hold the instruction we recorded (wrong version/region) is left alone
// instead of corrupted. Originals were read from each address in the Hex Editor on USA v1.1.0.
typedef struct { u8 ch; u32 addr, orig, patch; } CodePatch;
static const CodePatch MG_PATCHES[] = {
    { CH_MG_TOWN_GALLERY,  0x004E441C, 0xE2011001, 0xE2811032 }, // AND r1,r1,#1  -> ADD r1,r1,#0x32
    { CH_MG_SWAMP_GALLERY, 0x004FA838, 0xE281201E, 0xE2812B1E }, // ADD r2,r1,#0x1E -> #0x7800
    { CH_MG_BEAVER,        0x0036A358, 0xEA000004, 0xE1A00000 }, // B    -> NOP
    { CH_MG_BEAVER,        0x0036A360, 0x0A000002, 0xE1A00000 }, // BEQ  -> NOP
    { CH_MG_BOAT_JUMP,     0x00171780, 0xE0800001, 0xE3A00015 }, // ADD r0,r0,r1 -> MOV r0,#0x15
    { CH_MG_HONEY_DARLING, 0x00458774, 0xE3500001, 0xE2800000 }, // CMP r0,#1 -> ADD r0,r0,#0
};
#define NUM_MG_PATCHES ((int)(sizeof(MG_PATCHES) / sizeof(MG_PATCHES[0])))

// Write one word into a read-only game page (.text). The process-wide RWX flip (main.c) does NOT
// make .text writable here - a plain store to it faults and is silently dropped - so we alias the
// target's physical page at a scratch RW virtual address and write through that. Get a free VA by
// allocating a page then freeing it (the address stays a valid hole to map into), map the target
// page there, store, flush that word back to physical RAM, unmap, then invalidate the I-cache at
// the real address so the CPU refetches the rewritten instruction.
static int PatchWord(u32 addr, u32 val)
{
    u32 page = addr & ~0xFFFu, off = addr & 0xFFFu, scratch = 0, dummy = 0;
    if (R_FAILED(svcControlMemory(&scratch, 0, 0, 0x1000, MEMOP_ALLOC, MEMPERM_READ | MEMPERM_WRITE)))
        return 0;
    svcControlMemory(&dummy, scratch, 0, 0x1000, MEMOP_FREE, 0);
    if (R_FAILED(svcMapProcessMemoryEx(CUR_PROCESS_HANDLE, scratch, CUR_PROCESS_HANDLE, page, 0x1000)))
        return 0;
    *(volatile u32 *)(scratch + off) = val;
    svcFlushDataCacheRange((void *)(scratch + off), 4);
    svcUnmapProcessMemoryEx(CUR_PROCESS_HANDLE, scratch, 0x1000);
    svcInvalidateInstructionCacheRange((void *)addr, 4);
    return 1;
}

static void ApplyCodePatches(void)
{
    static u8 rowOn[NUM_MG_PATCHES];       // per-row edge memory - patch/revert only on a change
    for (int i = 0; i < NUM_MG_PATCHES; ++i)
    {
        int on = cheatState[MG_PATCHES[i].ch] ? 1 : 0;
        if (on == rowOn[i]) continue;
        u32 want = on ? MG_PATCHES[i].patch : MG_PATCHES[i].orig;
        u32 cur  = R32(MG_PATCHES[i].addr);
        // Only touch a word we recognise - the recorded original, or our own patch (idempotent /
        // revert). Anything else is unknown code (wrong version); leave it alone.
        if (cur != want && (cur == MG_PATCHES[i].orig || cur == MG_PATCHES[i].patch))
            PatchWord(MG_PATCHES[i].addr, want);
        rowOn[i] = (u8)on;
    }
}

// Continuous cheats: applied every tick while the menu is CLOSED (game running).
// Keep this cheap - it runs at game framerate.
static void ApplyCheats(void)
{
    u32 pad = HID_PAD; // needed by the MM3D hold/scrub cheats below, ahead of the EXAMPLE guard

    // MM3D (USA v1.1.0, 0004000000125500). CONFIRMED on hardware via Cheat Search:
    // Known Value 27 -> 57, poked 0x776318, in-game rupee counter matched.
    if (cheatState[CH_MM_RUPEES_MAX]) W16(0x776318, 999);

    // Max Hearts / Enhanced Defense: the write VALUES are confirmed on hardware (both worked
    // as plain writes). The revert-on-off behavior below is new and not yet confirmed: capture
    // the real value on the rising edge (first frame it's turned on), hold it while on, put the
    // captured value back on the falling edge (once, when it's turned off) - so turning the
    // cheat off restores whatever you actually had before enabling, not a guess.
    {
        static u16 savedCap = 0, savedCur = 0; static u8 wasOnHearts = 0;
        int on = cheatState[CH_MM_HEARTS_MAX];
        if (on && !wasOnHearts) { savedCap = R16(0x776312); savedCur = R16(0x776314); }
        if (on) { W16(0x776312, 0x0140); W16(0x776314, 0x0140); }
        else if (wasOnHearts) { W16(0x776312, savedCap); W16(0x776314, savedCur); }
        wasOnHearts = (u8)on;
    }
    {
        static u8 savedDef = 0, wasOnDef = 0;
        int on = cheatState[CH_MM_DEFENSE];
        if (on && !wasOnDef) savedDef = R8(0x776320);
        if (on) W8(0x776320, 0x01);
        else if (wasOnDef) W8(0x776320, savedDef);
        wasOnDef = (u8)on;
    }

    // Time / Day Scrub - hold {HK} + D-Pad to nudge. CONFIRMED on hardware, forward direction
    // reliable both ways; backward inherits the same known day-rollover/transition-gating
    // limitation as the one-shot Set Time / Set Day cheats (see their comments below).
    if (cheatState[CH_MM_TIME_SCRUB] && (pad & hotKeys[hk1].mask))
    {
        if (pad & BUTTON_UP)   W8(0x7761F9, (u8)(R8(0x7761F9) + 2));
        if (pad & BUTTON_DOWN) W8(0x7761F9, (u8)(R8(0x7761F9) - 2));
    }
    {
        static u32 prevDayScrub = 0;
        u32 edge = pad & ~prevDayScrub;
        if (cheatState[CH_MM_DAY_SCRUB] && (pad & hotKeys[hk2].mask))
        {
            if (edge & BUTTON_UP)   { u8 d = R8(0x7761EC); W8(0x7761EC, (u8)(d >= 3 ? 1 : d + 1)); }
            if (edge & BUTTON_DOWN) { u8 d = R8(0x7761EC); W8(0x7761EC, (u8)(d <= 1 ? 3 : d - 1)); }
        }
        prevDayScrub = pad; // always track, so a stale edge can't fire on hotkey reacquire
    }

    // Moon Jump - CONFIRMED on hardware. First base+offset cheat in this plugin: the pointer
    // chain (0x08363784 -> +0x4AC -> player actor) came from the AR code's own decode, never
    // independently re-derived, and it works. Both pointer levels stay null-checked.
    if (cheatState[CH_MM_MOONJUMP] && (pad & BUTTON_L1) && (pad & BUTTON_A))
    {
        u32 base = R32(0x08363784);
        if (base)
        {
            u32 p = R32(base + 0x4AC);
            if (p) W16(p + 0x6A, 0x4100);
        }
    }

    // Ammo max/inf toggles - CONFIRMED on hardware (see ammoItems[] above).
    if (cheatState[CH_MM_AMMO_ARROWS]) W8(0x776391, 0x63);
    if (cheatState[CH_MM_AMMO_BOMBS])  W8(0x776396, 0x63);
    if (cheatState[CH_MM_AMMO_CHUS])   W8(0x776397, 0x63);
    if (cheatState[CH_MM_AMMO_STICKS]) W8(0x776398, 0x63);
    if (cheatState[CH_MM_AMMO_NUTS])   W8(0x776399, 0x32);
    if (cheatState[CH_MM_AMMO_BEANS])  W8(0x77639A, 0x63);
    if (cheatState[CH_MM_AMMO_KEG])    W8(0x77639C, 0x63);

    ApplyCodePatches();  // minigame instruction patches - MUST run before the EXAMPLE guard below

    // Guard, not #if: the example bodies below stay COMPILED (so they cannot silently rot
    // as the engine changes) while -Os folds them away entirely until you flip the flag.
    if (!EXAMPLE_ENABLED) return;

    // EXAMPLE - direct write. Pins a value for as long as the cheat is on.
    // W8 / W16 / W32 pick the width; match whatever the game actually stores there.
    if (cheatState[CH_EX_DIRECT])
        W16(EXAMPLE_ADDR_DIRECT, EXAMPLE_VALUE_DIRECT);
    if (cheatState[CH_EX_BYTE])
        W8(EXAMPLE_ADDR_DIRECT, 0x63);           // 99, the classic "max this counter"
    if (cheatState[CH_EX_WORD])
        W32(EXAMPLE_ADDR_DIRECT, 0x0000270F);    // 9999

    // EXAMPLE - base+offset write. ALWAYS null-check the base before writing through it.
    if (cheatState[CH_EX_BASEOFF])
    {
        u32 base = ExampleBase();
        if (base) W32(base + EXAMPLE_OFF_FIELD, EXAMPLE_VALUE_FIELD);
    }

    // EXAMPLE - hold-to-act, using the player's rebindable hotkey instead of a fixed button.
    if (cheatState[CH_EX_HOTKEY] && (pad & hotKeys[hk1].mask))
    {
        u32 base = ExampleBase();
        if (base) W32(base + EXAMPLE_OFF_FIELD, EXAMPLE_VALUE_FIELD);
    }
}

// Real RGBA4444 sprites (sprites.h), ripped from The Spriters Resource: Item Icons by
// Colbydude, UI (rupee) by xAct. See Tools/gen_sprites_mm3d.py for exactly which sheet cell
// each one comes from, and why the non-obvious ones (Fairy, Shield, Notebook) were picked.
// Defined up here (rather than by SpriteKeyForCheat further down) because the Picker option
// tables below also reference these.
#define MSPR_ARROWS       0x100
#define MSPR_BOMBS        0x101
#define MSPR_BOMBCHUS     0x102
#define MSPR_STICKS       0x103
#define MSPR_NUTS         0x104
#define MSPR_BEANS        0x105
#define MSPR_KEG          0x106
#define MSPR_SWORD        0x107
#define MSPR_QUIVER       0x108
#define MSPR_HEART        0x109
#define MSPR_MAGIC_FAIRY  0x10A
#define MSPR_DEFENSE      0x10B
#define MSPR_ALL_ITEMS    0x10C // Bombers' Notebook - now the Quest FOLDER icon
#define MSPR_ALL_MASKS    0x10D
#define MSPR_OCARINA      0x10E // now the Time FOLDER icon
#define MSPR_FAIRY        0x10F
#define MSPR_RUPEE        0x110
#define MSPR_WALLET       0x111 // Inventory FOLDER icon
#define MSPR_CAMERA       0x112 // Misc FOLDER icon (Pictograph Box - playful, novelty-flavored)
#define MSPR_ITEMS_DEED   0x113 // Have all Items cheat (freed up from MSPR_ALL_ITEMS)
#define MSPR_BOSS_REMAINS 0x114 // All Bosses and Songs cheat (freed up from MSPR_OCARINA)
#define MSPR_MOONJUMP     0x115 // (tentative ID) Moon Jump - no boot icon on this sheet
#define MSPR_LENS         0x116 // Lens of Truth - Cheat Search
#define MSPR_MAP          0x117 // Dungeon Map - RAM Dumper
#define MSPR_COMPASS      0x118 // Compass - Hex Editor
#define MSPR_GARO_MASK    0x119 // Garo's Mask - Change Theme
#define MSPR_SCROLL       0x11A // Trade Quest scroll - Language
#define MSPR_ABOUT_ICON   0x11B // Majora's Mask HOME icon crop - About
#define MSPR_BOTTLE       0x11C // empty Bottle - Bottle #1-7 pickers
#define MSPR_FISHINGROD   0x11D // Fishing Rod - Fishing Hole Pass
#define MSPR_GFSWORD      0x11E // Great Fairy's Sword - B Button picker
// Per-option Bottle picker icons (see gen_sprites_mm3d.py for the sheet-cell notes).
#define MSPR_B_REDPOTION   0x120
#define MSPR_B_GREENPOTION 0x121
#define MSPR_B_BLUEPOTION  0x122
#define MSPR_B_FAIRY       0x123
#define MSPR_B_DEKUPRINCESS 0x124
#define MSPR_B_MILK        0x125
#define MSPR_B_FISH        0x126
#define MSPR_B_BUG         0x127
#define MSPR_B_BIGPOE      0x128
#define MSPR_B_SPRINGWATER 0x129
#define MSPR_B_HOTSPRING   0x12A
#define MSPR_B_GOLDDUST    0x12B
#define MSPR_B_MUSHROOM    0x12C
#define MSPR_B_SEAHORSE    0x12D
#define MSPR_B_CHATEAU     0x12E
#define MSPR_B_MYSTMILK    0x12F
#define MSPR_B_ZORAEGG     0x132
#define MSPR_ZORA_MASK     0x130 // Play as... folder
#define MSPR_FD_MASK       0x131 // Play as... folder
// 100% Checklist icons
#define MSPR_EQ_RAZOR      0x134
#define MSPR_EQ_HERO       0x135 // Hero's Shield - Mirror Shield reuses MSPR_DEFENSE (same cell)
#define MSPR_EQ_BOMBBAG1   0x136
#define MSPR_EQ_BOMBBAG2   0x137
#define MSPR_EQ_BOMBBAG3   0x138
#define MSPR_EQ_QUIVER2    0x139
#define MSPR_EQ_QUIVER3    0x13A
#define MSPR_EQ_WALLET2    0x13B
#define MSPR_OWL_ICON      0x13C // list rows
#define MSPR_OWL_MODEL     0x13D // Checklist detail card only (see iconArgBig)
// Individual Mask icons (22 of 24 real, from Zelda Wiki; Bunny Hood/Keaton cropped from our own
// sheet - see gen_sprites_mm3d.py). Zora/Fierce Deity reuse MSPR_ZORA_MASK/MSPR_FD_MASK.
#define MSPR_MASK_DEKU       0x140
#define MSPR_MASK_GORON      0x141
#define MSPR_MASK_TRUTH      0x142
#define MSPR_MASK_KAFEI      0x143
#define MSPR_MASK_ALLNIGHT   0x144
#define MSPR_MASK_BUNNY      0x145
#define MSPR_MASK_KEATON     0x146
#define MSPR_MASK_ROMANI     0x147
#define MSPR_MASK_TROUPE     0x148
#define MSPR_MASK_POSTMAN    0x149
#define MSPR_MASK_COUPLE     0x14A
#define MSPR_MASK_GREATFAIRY 0x14B
#define MSPR_MASK_GIBDO      0x14C
#define MSPR_MASK_DONGERO    0x14D
#define MSPR_MASK_KAMARO     0x14E
#define MSPR_MASK_CAPTAIN    0x14F
#define MSPR_MASK_STONE      0x150
#define MSPR_MASK_BREMEN     0x151
#define MSPR_MASK_BLAST      0x152
#define MSPR_MASK_SCENTS     0x153
#define MSPR_MASK_GIANT      0x154
// Individual Boss Remains icons (100% Checklist Bosses category) - real per-boss sprites from
// Zelda Wiki, replacing the shared MSPR_BOSS_REMAINS placeholder.
#define MSPR_BOSS_ODOLWA     0x155
#define MSPR_BOSS_GOHT       0x156
#define MSPR_BOSS_GYORG      0x157
#define MSPR_BOSS_TWINMOLD   0x158
