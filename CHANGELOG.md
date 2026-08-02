# Changelog — MajoraCTRComposer (Zelda: Majora's Mask 3D)

A self-rendered `.3gx` overlay plugin for The Legend of Zelda: Majora's Mask 3D
(New 3DS · **USA**, Title ID `0004000000125500`, version **1.1.0**). Version numbers follow
SemVer; the **build** counter is the running iteration count shown on-screen (`bNN`).

---

## Unreleased · builds 1–54

### b54 — Checklist hints cleaned of "MM3D reward:"/N64-comparison text
7 hint strings in `plugin/tracker_data.inc.c` simplified to plain MM3D-only descriptive text,
removing the "MM3D reward:" prefix and any N64-comparison clause ("swapped from N64's...", "was
a Bottle in N64", etc.): Troupe Leader's Mask, Woodfall/Snowhead Stray Fairies, Dampe's grave
dig, South Clock Town owl statue, Koume's Boat-Cruise bottle, Double Magic equipment. Source
comments (not shown in-game) still mention N64 where relevant for context - only player-facing
hint text was in scope.

### Stray Fairies checklist auto-fill CONFIRMED on hardware (no code change)
- Open since build 40 ("Stray Fairies still reading 0/4 against a real 100% save is unresolved").
  User applied "All Stray Fairies", saved, reloaded - the Checklist correctly read all 4 dungeon
  entries as done (4/5, only the manual Clock Town fairy left). Confirms both the address
  (0x7763E8-0x7763EB) and the auto-fill round-trip through an actual save file. Comment updated
  in `plugin/tracker_data.inc.c`; no functional change.

### b53 — 22 cheats/pickers marked CONFIRMED on hardware
User tested every remaining "not yet confirmed" cheat and picker and confirmed all of them work:
Fill Rupee Bank, Razor Sword + Mirror Shield, Gilded Sword / Great Fairy Sword (B Button), all 7
Bottle pickers, all 7 Max/Inf ammo toggles, and Have all Items / Have all Masks / All Bosses and
Songs / All Stray Fairies. In-menu descriptions and source comments updated accordingly. Also
fixed two stale section-header comments (Battle folder, Ammo folder) that still said "not yet
confirmed" despite every cheat inside them individually already saying CONFIRMED.

### b52 — Owl Statues "Locations" header uppercase, matching CLOCK TOWN/SWAMP/etc.

### Investigation update: Cheat Search narrowing for Owl Statues did not converge (no code change)
- Second live attempt using the refined Increased/Unchanged-while-idle method (to kill drift from
  unrelated live-changing values) still landed on 11 surviving addresses after 13 steps, none in
  the `0x776000`-`0x779000` range where every other confirmed save field lives. Left open; not
  pursued further this session.

### b51 — Owl Statues list correction: Snowhead, not Goron Village
- The 10th real owl statue is **Snowhead**, not Goron Village (user correction to the original
  list). `isOwl` flipped: Goron Village (Spring/Winter) → 0, Snowhead → 1, with the "(from owl
  statue)" description text moved to match. The Owl Statues filter's ordered block now shows
  Snowhead (one row - it has no Spring/Winter split) in place of the two Goron Village rows,
  between Mountain Village and Great Bay Coast.

### b50 — Owl Statues "Locations" header + grid header style fixed to match the list
- Owl Statues filter now shows a "Locations" section header (dim label + gold hairline) between
  the Filter row and the destinations, matching how Overworld/Dungeons show "CLOCK TOWN" etc.
  Implemented as a raw `Item` literal (not `IT_SEP()`, which hardcodes `desc=NULL`) carrying the
  `OWL_ORDER_MARK` tag so it's shown only under that filter, same as the rows below it.
- **Found and fixed a real bug while matching the styles**: the 2-column grid layout (used by the
  "All" Teleport filter and HOME) drew section headers with hardcoded RGB triples
  (`150,140,112` / `120,98,50`) instead of the theme-aware `INK_DIM`/`GOLD` macros the
  single-column list uses (`engine/menu_render.inc.c`, `DrawMenuItem`'s `IS_SEP` branch). Every
  grid-mode header (HOME's own section headers included, not just Teleport's) was rendering in a
  fixed color instead of following the active theme. Now both paths call the same macros.

### b49 — Owl Statues filter now follows the requested visiting order
- The Owl Statues filter (build 48) listed entries in the same geography-grouped order as
  Overworld/Dungeons, since it reused the same rows. Now it shows the user-supplied order
  instead: South Clock Town, Milk Road, Southern Swamp, Woodfall, Mountain Village, Goron
  Village, Great Bay Coast, Zora Cape, Ikana Canyon, Stone Tower.
- Implementation: `teleportItems[]` (`plugin/menu_tables.inc.c`) gained a duplicate block of the
  same 12 `IT_WARP` rows (Mountain Village and Goron Village each keep their Spring/Winter scene
  variant) laid out in that exact order, tagged via a sentinel pointer (`OWL_ORDER_MARK`) in the
  otherwise-unused `Item.desc` field (warp rows always pull their real label/desc from `warps[]`
  at draw time, so `desc` was free to repurpose). `ItemHidden()` (`engine/menu_nav.inc.c`) shows
  that block ONLY under the Owl Statues filter and hides it everywhere else, so it never
  duplicates entries under "All". `Warp.isOwl` no longer drives the filter directly - it stays as
  the documented, checkable source of which warps are real owl statues.

### b48 — Owl Statues filter in Teleport
- Added a 4th Teleport filter category, cycled by pressing {A} on the filter row: All ->
  Overworld -> Dungeons -> **Owl Statues**. Matches all 10 real MM3D owl statue warp points
  (user-supplied list): South Clock Town, Southern Swamp, Woodfall, Mountain Village (both
  Spring and Winter variants), Goron Village (both variants), Great Bay Coast, Zora Cape, Ikana
  Canyon, Stone Tower, Milk Road.
- New `isOwl` flag on the `Warp` struct (`plugin/pickers.inc.c`). While auditing it, Snowhead's
  description was found to wrongly claim "(from owl statue)" - it's not on the real list, so that
  claim was removed; Goron Village's description was missing the same claim even though it IS a
  real owl statue, so that was added.

### b47 — first Bomber's Notebook event auto-detected and CONFIRMED on hardware
- The `0x777469` candidate address from the earlier investigation note is real: re-diffed all 65
  `SaveGames/` saves for single-bit transitions landing in a window with exactly one identifiable
  event, tested the strongest candidate live (load `SaveGames/Saves09/save0.bin`, Hex Editor read
  `0x00` at `0x77746D`, get Epona at Milk Road, re-read `0x10`) - **confirmed**.
- **"A Race near Milk Road" (`note_16`) is now auto-detected**: `CK_BIT, 0x77746D, 0x10`.
- **Second candidate tested and RULED OUT**: bit 55 (byte `0x77746F`, mask `0x80`) looked strong
  offline - the only bit changing between save steps "15-3" and "16-1", whose notes say "Frog
  Choir", matching `note_47` ("Reunite the Frog Choir") by name - but the live test (Hex Editor
  read `0x59` before AND after completing the quest) showed no change. The name match was
  coincidental. `note_47` stays manual; do not re-propose this address for it.
- The other 61 events still have no known bit - most single-bit windows in the corpus contain
  more than one candidate event, so they were not guessed at.

### b46 — individual Boss Remains sprites in the 100% Checklist
- All 4 Bosses (Odolwa, Goht, Gyorg, Twinmold) now show their own real Boss Remains icon instead
  of a shared placeholder (`MSPR_BOSS_REMAINS`, "Odolwa's Remains" reused for all four). Sourced
  from Zelda Wiki (zeldawiki.wiki, cdn.wikimg.net), same provenance as the 22 individual Mask
  icons. `MSPR_BOSS_REMAINS` itself is unchanged and still used by the "All Bosses and Songs"
  cheat's own icon.

### Source reorganization: `main.c` split into `plugin/` + `engine/` (no build change)

Followed `references/MIGRACAO-CTRCOMPOSER.md` to bring this fork in line with the CTRComposer
template's own restructuring. `RawPlugin/sources/main.c` went from **6,680 lines to 257** — now
just the header includes, `plugin/identity.inc.c`, the ordered chain of `#include`s, and
`Thread / entry` (`PluginShutdown`/`ThreadMain`/`main`). Everything else moved into 30 files
under two new directories, each `#include`d back in at the exact point it was cut from:

- **`sources/plugin/`** (8 files) — the game-specific half: cheat IDs/implementations, pickers
  (bottle contents, warps), menu data tables, cheat icon selection, the 190-item Checklist data
  (`CHK_MASKS`…`CHK_CATS`), and the MM3D guide text/credits.
- **`sources/engine/`** (22 files) — the reusable half: platform/render/storage plumbing, the
  menu model and its renderer, theming, sprites/icons, tools (Cheat Search, RAM Dumper, Hex
  Editor), the guide reader, the Checklist's generic type/UI machinery, and the menu/quick-menu
  loops.

Every single cut was a straight `#include`-based move — same translation unit, same token stream
— verified after each one with `cmp` against a pre-migration baseline `.3gx` and with
`Tools/fingerprint.sh` (symbol name+size, order-independent). **The final binary is
byte-identical to what shipped as b45 before this reorganization** (`cmp` passed on the very last
rebuild too, including for the one section — Game Guide/Plugin Guide, where `PLUGIN_PAGES[]`
used to appear twice under `#if TOOLS_ONLY`/`#else` — that involved a deliberate reorder). 22
migration commits, one per cut, each independently buildable and revertible.

Two things worth remembering for next time: a quoted `#include` inside a file that itself lives
in a subdirectory resolves relative to THAT file's own directory, not to `main.c` — one cut
briefly broke on this until the nested include's path grew a `../`. And where a single named
section in the original file actually mixed engine machinery with game data (`Menu model`,
`Sprites`, `Completion tracker`, the Guide), the migration guide's own table already called for
splitting it 2-4 ways rather than moving it as one block — followed literally rather than
improvised.

**Re-tested on hardware after the reorganization — confirmed working normally**, matching the
byte-identical binary's own evidence that nothing actually changed.

### b45 — engine fixes inherited from CTRComposer (`references/CORRECOES-MOTOR.md`)

Four defects that live in the engine itself, not in this plugin's own code, so every CTRComposer
fork has them. All four applied here and **CONFIRMED on hardware in this fork** (b45): the tool
-> quick menu -> exit path hands the top screen back, the menu backdrop no longer stacks copies
of the quick-menu panel, held buttons release on their own, and the normal
`SELECT` / `L+SELECT` paths did not regress.

- **Top screen froze on exit (🔴 breaks usage).** `Present()` flips
  `REG32(LCD_TOP + LCD_SELECT)` to show our frame and nothing ever flipped it back, so on exit
  the LCD kept scanning the *plugin's* buffer: the game ran on underneath (bottom screen
  returned, audio played) while the top stayed frozen on the last menu frame. Only the top breaks
  because the bottom draws straight into the visible buffer, where restoring pixels is enough.
  Added `TopTakeOver()` / `TopRelease()` around the register, called from `RunMenu()` and
  `QuickMenu()`. The quick menu only hands back when it is really returning to the game — if a
  favorite folder or tool was picked, `RunMenu()` takes over immediately and handing back would
  cause a one-frame flicker. Repro was narrow (star a *tool*, launch it from the quick menu,
  exit), which is why it can sit unnoticed in a fork whose favorites are all cheats.
- **Quick menu got baked in as the menu backdrop.** Coming from the quick menu, `RunMenu()`
  called `GrabFb()` microseconds after `ResumeGame()` — before the game had drawn anything — so
  it captured our own panel as the "game frame" and each reopen stacked another copy. Added a
  `g_qmHandoff` flag; on that path we `RestoreTopBackdrop()` instead of re-grabbing.
- **Unbounded button waits (🟡 latent).** Four raw `while (HID_PAD ...)` loops (info box exit,
  About exit, menu entry, quick-menu entry) spun forever on a stuck pad — **with the game
  paused**, which presents as a dead console. All four now call the engine's existing capped
  `DrainButtons()` (~2s). `DrainButtons` moved up next to `ARepeat`, since the info box and About
  sit earlier in the file than the menu and could not see it — which is why they had grown their
  own uncapped waits in the first place.
- **Translated text could overflow stack buffers (🟡 latent).** `T()` resolves to a `.txt` on the
  SD card, so its length is outside the engine's control, yet it was formatted into fixed-size
  stack buffers with `siprintf`. Converted every call whose format takes a `T(...)` or an
  author-written label to `sniprintf` with the destination size (`sniprintf`, the integer-only
  variant, not `snprintf` — the latter would pull the float formatter into the binary).
  **Beyond the upstream note:** four of these feed their return value straight into
  `FSFILE_Write` (Favorites.txt, Checklist.txt). `sniprintf` returns the length it *wanted* to
  write, so a long label would have made the write run off the end of the buffer — those four
  now clamp the length before using it.
- **`{D-Pad}` was not a real glyph token.** `GlyphTok()` matches `{DP}` exactly, so the Plugin
  Guide footer rendered the braces as literal text. Fixed, and the same string was the only
  footer without `T()` — now translatable too. Audited every `{token}` in the source against the
  accepted set (`{A} {B} {X} {Y} {L} {R} {DP} {HK}`); no others were wrong.
- **Warnings turned on.** The Makefile was building without `-Wall -Wextra`. Enabled, now at
  **zero warnings**, with three deliberate suppressions: `-Wno-main` (the entry point is not a
  hosted `main`), `-Wno-missing-field-initializers` (the Checklist `ChkItem` tables initialise
  only leading fields on purpose — the omitted tail is meant to be zero), and `(void)arg` in
  `ThreadMain`. Four vector tool icons (`MagnifierIcon`, `DiskIcon`, `GridIcon`, `InfoIcon`)
  turned out to be unreferenced — superseded by real MM3D sprites — and are marked
  `__attribute__((unused))` rather than deleted, since they are the engine's art-free fallback.
  `--gc-sections` already keeps them out of the binary.

### Investigation note: a Bomber's Notebook auto-detect candidate (no code change)
- Went looking for more auto-detectable save data by diffing all 65 files in `SaveGames/`
  (progressive saves 1-1 through 22-1) against `SaveGames/MM3D Saves.txt`'s per-step notes,
  after the zeldaret/mm decompilation turned up a `weekEventReg[100]` array with an individual
  bit per Bomber's Notebook event on N64 (`include/z64save.h`) - promising, but MM3D expanded
  the Notebook from N64's ~20 people to 63 events, so the N64 byte/bit layout can't be assumed
  to carry over directly.
- Found a strong **candidate address: `0x777469`, 8 bytes**. Unlike most monotonic byte regions
  in the save data (which flip in a few large batches - consistent with derived/checksum data,
  not individual events), this one flips at 16 different save-steps across the corpus, and the
  number of newly-set bits matches the number of newly-listed achievements in the notes exactly
  at some steps (e.g. step "1-2": 4 new bits for 4 listed achievements; step "2-2": 6 new bits
  for 6 listed achievements) - not a perfect match at every step, but well beyond coincidence.
  It's also present, byte-identical, in the save data's known second struct copy (`0x778E53` -
  the same `+0x1A88` offset already established for the primary/secondary save copy split),
  which rules out random noise.
- **Not implemented**: mapping which specific bit is which specific Notebook event needs
  hardware testing (complete one named event at a time, watch which bit flips via Hex Editor) -
  deferred at the user's request. The address is documented here so the next session doesn't
  have to re-derive it.

### Decoded the quest bitfield: Bosses + 11 more auto-detected songs (build 44)
- Cross-referenced the zeldaret/mm N64 decompilation's `QuestItem` enum (24 entries, indices
  0x00-0x17) against our own CONFIRMED 3-byte quest field (0x7763D0-0x7763D2). The indices line
  up exactly with 24 bits across those 3 bytes (byte0=bits 0-7, byte1=8-15, byte2=16-23, LSB
  first) - decoding the confirmed 100%-save value (`CF F7 CF`) against that layout lines up
  perfectly for all 4 bosses and 11 of the 13 songs, which is strong (if indirect) evidence the
  bit order is right.
- **New "Bosses" category** (Odolwa, Goht, Gyorg, Twinmold), fully auto-detected.
- **Songs auto-detection jumped from 0/13 to 11/13** - only Inverted Song of Time and Song of
  Double Time stay manual, since they're just the Song of Time played differently rather than
  separately-learned songs with their own flag.
- Not hardware bit-tested (same caveat as everything else added this way) - the byte-level "does
  this equal 0xCF/0xF7/0xCF" cheat IS confirmed; the individual bits within it are a decode, not
  a verified read. A few other bits in that field (quiver/bomb bag/heart piece/notebook-related)
  didn't decode consistently against the 100% save, so those were left alone rather than guessed.

### Individual mask icons, real song colors, bottle sprites instead of swatches (build 43)
- **All 24 Masks now show their own real icon** instead of one shared Bunny Hood placeholder.
  22 came from Zelda Wiki's individually-named icon files (e.g. `MM3D_Deku_Mask_Icon.png`),
  cross-checked visually cell-by-cell against our own sheet. Two of those (Bunny Hood, Keaton
  Mask) turned out to both redirect to the exact same unrelated icon on the wiki - a wiki data
  issue, not a game asset - so those two are cropped from our own sheet instead, using the cells
  we'd already visually identified earlier. This same research also reordered several masks we'd
  guessed wrong before (e.g. what we thought was Blast Mask is Mask of Truth; the "cow face" we
  assumed was Bremen Mask is actually Romani's Mask; Bremen Mask is a white bird).
- **Song note colors** switched from an invented OoT-style area palette to a best-effort match of
  the real in-game Ocarina Songs screen's actual color badges (yellow, red, blue, purple, green,
  orange, plus several identical cyan ones) - still not a confirmed per-song mapping (no source
  individually labels which exact song gets which color the way Masks did), so it's flagged as
  inferred in the code comment, not asserted as fact.
- **Bottle checklist entries with no matching content sprite** (Koume's Boat-Cruise, Beaver
  Brothers race, Madame Aroma's mail) switched from a plain color swatch to an actual bottle
  sprite picked for a playful/punny fit (a bottled fish for a race in a waterfall cave, a plain
  bottle for "message in a bottle" mail delivery) - closer to what was asked for than a flat color.

### Real sprites throughout the 100% Checklist (build 42)
- Added `CKI_SPRITE` (a real sprites.h icon instead of a hand-drawn bitmap) and `CKI_SWATCH` (a
  plain color-tinted square, for the handful of items nothing on the sheet matches) to the
  Checklist's icon system, plus an optional `iconArgBig` field so a category can show a different,
  bigger image in the detail card than in list rows.
- **Masks** → the Bunny Hood (shared "masks" icon). **Heart Pieces** → the Heart Container.
  **Stray Fairies** → the Stray Fairy sprite. **Bomber's Notebook** → the Notebook itself, for
  all 63 entries. **Equipment** → real per-item art (Gilded Sword, Mirror/Hero's Shield, Great
  Fairy's Sword, all 3 Quiver tiers, all 3 Bomb Bag tiers, both Wallet tiers - Bomb Bag and Quiver
  tiers came from an unused row on the sheet with 3 size variants each). **Bottles** → the actual
  bottle-content icon where one exists (Kotake's Red Potion, Chateau Romani, Gold Dust, Fraternal
  Milk); the 3 with no bottle equivalent (an archery minigame, a race, a mail delivery) get a
  plain color swatch in a shade that matches the reward's theme instead.
- **Songs** keep the existing colored-note icon (not a flat sprite) and now use a color per song
  matched to its area/dungeon (Woodfall/Deku green, Snowhead/Goron red, Great Bay/Zora blue,
  Ikana purple, Oath to Order gold) - these are thematic choices, not a confirmed in-game color.
- **Owl Statues** get a real 2-tier treatment: the actual small in-game icon in list rows, and a
  separate, bigger 3D-render crop in the Checklist's detail card (both from The Spriters
  Resource / Zelda wiki assets) - the first checklist category to use `iconArgBig`.

### 100% Checklist expanded from 32 to 190 items (build 41)
- Researched and cross-checked a full MM3D-specific 100% completion list (not the N64 original -
  several categories differ) across multiple sources. New categories: **Heart Pieces** (52, all
  manual - no known per-piece save address), **Songs** (13 - MM3D added Song of Storms, one more
  than N64's 12), **Bomber's Notebook** (63 events - MM3D completely overhauled this from N64's
  fixed 20-person list), **Owl Statues** (10), **Bottles** (7 acquisition sources - MM3D added a
  Gorman "Fraternal Milk" sidequest and swapped Koume's archery reward from a Heart Piece to a
  Bottle), **Equipment** (swords/shields/quiver/bomb bag/wallet tiers - folds in the previous
  "Gear" category's 4 auto-detected entries plus 11 more manual ones). Corrected **"Circus
  Leader's Mask" to "Troupe Leader's Mask"** (the MM3D UI's actual name for it) in the Masks list.
- **Stray Fairies**: added the Clock Town fairy (the 61st, easy to miss - the save data has no
  address for it so it's manual) and corrected the per-dungeon reward text - MM3D swapped
  Woodfall's and Snowhead's Great Fairy rewards (Great Spin Attack and Double Magic) from their
  N64 positions.
- Everything without a hardware-confirmed save address is `CK_MANUAL` (tick it yourself) rather
  than guessed at - only Masks, Stray Fairies (per-dungeon) and 4 Equipment entries auto-fill.

### Fixed Masks reading 0/24 against a real 100% save (build 40)
- **Root cause**: the 24-byte mask array (0x77636C-0x776383) fills in ACQUISITION order, not one
  fixed slot per mask - the earlier `CK_BYTEEQ`-per-fixed-offset detection only happened to work
  against the one 100% save used during Phase 1/2 analysis, which coincidentally acquired masks
  in ascending id order. A real save can have any mask id in any of the 24 slots. Added a new
  detection kind, `CK_SCANEQ` (does this id appear ANYWHERE in the 24-byte range, not at one
  fixed offset), and switched all 24 mask entries to it.
- **Renamed** "Tracker" to "100% Checklist" throughout the menu, and gave it a real sprite icon
  (the Bombers' Notebook - MM3D's own in-game checklist item) instead of the hand-drawn
  placeholder vector.
- **Bottle picker**: added icons for the 6 options that were missing one (Blue Fire, Poe, Big
  Poe, Zora Egg, Hylian Loach, Obaba's Drink) - Zora Egg gets a new sprite from the sheet: the
  rest reuse a thematically-close existing icon (a fish for Hylian Loach, the ghostly glow bottle
  for both Poe types, etc.) since the sheet has no distinct art for them.
- **Stray Fairies still reading 0/4 against a real 100% save is unresolved** - unlike Masks, this
  address (0x7763E8-0x7763EB) was never independently cross-checked against an actual save file,
  only assumed from the AR code's own claim. Needs a Hex Editor check against a real 100% save to
  confirm whether the address or the expected value (0x0F per dungeon) is wrong.

### Removed Deku Sticks Always On Fire (build 39)
- Tested on hardware: the write goes through (no crash) but has no visible effect - the stick
  stays unlit. Either the address is wrong or it doesn't control what the AR list claimed.
  Removed rather than leave a dead row in the menu; would need a real Cheat Search pass to
  find the right address before trying again.


### Bottles folder back to a vertical list (build 38)
- The 2-column grid clipped both the bottle label and its content value badly (e.g. "Green
  Potion" next to "Bottle #5" in a half-width column). Reverted to a plain single-column list,
  like every other folder besides HOME/unfiltered Teleport - full names, no clipping.


Boots and menu confirmed on real MM3D v1.1.0 hardware at build 1; every cheat below was
individually confirmed on the same console unless noted otherwise.

### 100% Checklist (Phase 5) + a real bug found in the Bottle picker (build 37)
- **Tracker now tracks real MM3D progress** instead of shipping the template's placeholder
  "Examples" category: **Masks** (24, one per byte in the confirmed 0x77636C-0x776383 range),
  **Stray Fairies** (4, one per dungeon, the confirmed 0x7763E8-0x7763EB range), and **Gear**
  (Enhanced Defense, Gilded Sword, Magic, Double Magic - all four already-confirmed cheat
  addresses reused as read checks). None of this is individually hardware-confirmed yet (the
  auto-fill has never been run against a real save), so treat it as a strong first pass, not
  gospel - Boss/Song tracking is deliberately left out for the same reason (see below).
  Auto-fill only *reads* memory, never writes, so there's no risk running it.
- **Found while researching mask ids for the Tracker**: cross-checked our mask/item ids against
  the real `ItemId` enum in the [zeldaret/mm](https://github.com/zeldaret/mm) N64 decompilation
  (`include/z64item.h`) - and it matches our own independently-confirmed B-Button value for
  Fierce Deity's Mask (0x35) exactly, so it's a trustworthy source. It also revealed **the
  Bottle picker's community-AR-list values were wrong from "Blue Fire" (0x1C) onward** - what we
  labeled "Big Poe" was actually Blue Fire (real Big Poe is 0x1E, not offered before), and
  "Mystery Milk"/"Mouldy Milk" (0x26/0x27) don't exist - those ids are really Hylian Loach and
  Obaba's Drink. **Fixed**: the Bottle picker now has the correct 22 real bottle contents in the
  right order (added Milk (Half), Blue Fire, Poe, Big Poe, Zora Egg, Hylian Loach, Obaba's Drink;
  dropped the two fictitious milk variants). The picker screen also gained scrolling, since 22
  options no longer fit in one un-scrolled page.
- **Boss/Song tracking not implemented**: the decomp's `QuestItem` enum confirms bosses and
  songs live as individual bits somewhere in the confirmed 3-byte 0x7763D0-0x7763D2 field, but
  without knowing the exact bit order (LSB vs MSB, which byte holds which range) a guess could
  easily mislabel which bit is Odolwa vs which is a song - worth a dedicated Cheat Search pass
  rather than shipping a guess.
- **Skipped for now** (same reasoning as build 36's Bomber's Code/Lotto): Heart Container count
  and the Quiver/Bomb Bag upgrade tier aren't in a simple byte-per-item shape we could add safely
  in this pass.

### More cheats from the AR list: Rupee Bank, Razor Sword, Deku Sticks on fire (build 36)
- **Fill Rupee Bank (5499)** (Inventory) - not yet confirmed. Address `0x777408` sits outside the
  mapped `0x776xxx` save block, but it's the same addressing style (no v1.0/v1.1 split) as the
  already-confirmed Fishing Hole Pass, so it's reasonably likely stable rather than heap-based.
- **Razor Sword + Mirror Shield** (Inventory) - not yet confirmed, but reuses the exact address
  the Gilded Sword cheat already confirmed (`0x776352`), just the other upgrade-tier byte value.
- **Deku Sticks Always On Fire** (Misc) - not yet confirmed. Fixed, code-adjacent address
  (`0x087DF99C`), not part of the heap-based save/GCTX structs.
- **Skipped for now**: Bomber's Code and the three Lotto-day codes from the AR list. Their
  addresses come in four different copies each (`0x6E22xx`/`0x6E3Cxx`/`0x7775xx`/`0x778Fxx`) that
  don't fit the confirmed `0x776xxx`+`0x1000` save-block pattern - the same "multiple copies,
  likely heap-based" shape that made the first three Teleport attempts silently do nothing before
  the stable GCTX pointer was found. Not worth guessing at without a hardware Cheat Search pass.

### Scoped the row-alignment fix to where it was actually needed (build 35)
- Build 33's checkbox-width alignment (`BrownBox()`) had been applied everywhere - including
  HOME, Settings, and Teleport, none of which asked for it and none of which needed it (HOME and
  Teleport are icon-only rows; Settings' gear-icon and checkbox rows already lined up with each
  other, since neither ever reserved the extra icon+checkbox combo space to begin with). That
  pushed every icon 17px right for no reason and put unwanted gaps in front of icons that used to
  hug the selector. `DrawMenuItem()` now takes an explicit `checkboxAlign` flag instead of always
  aligning: only **Inventory** passes it as on, since it's the one list that actually mixes
  generic checkbox+icon cheat rows (Max Rupees, Gilded Sword...) with folder/picker rows (Items,
  Bottles). Every other screen is back to its original tight layout.

### Un-nested Play as... and B Button Item (build 34)
- Build 33 had put both of these behind their own sub-folder (matching the Bottles folder
  pattern) - too much menu-inside-menu for what's just 2-3 rows each. Flattened back out:
  **Play as...** is a plain "FORMS" section inside Misc again (3 rows: Normal Link, Zora, Fierce
  Deity, still with build 33's real mask sprites); **B Button Item** is a non-selectable section
  header inside Inventory, directly above its two rows (Gilded Sword, Great Fairy Sword) - no
  more opening a picker screen for a 2-item choice, each row just applies directly like any other
  one-shot cheat.

### Row alignment, Play as.../B Button as their own categories, real Zora/Fierce Deity mask art (build 33)
- **Every row's icon and label now line up at the same x, regardless of row type.** Toggle/
  one-shot cheat rows reserve a checkbox-width slot before their icon; folders, pickers, tools,
  Teleport rows and the one-shot Settings rows didn't, so their labels started 17px to the left
  of every cheat row's label - visibly uneven columns (seen in the Inventory and Misc screens).
  Fixed by giving every non-toggle row type a solid black square, no border (a new `BrownBox()`,
  the main-menu-scale sibling of the quick menu's existing `BrownBoxS()`) in that same slot, so
  it reads as "not a toggle" while still keeping every label aligned.
- **"Play as..." is now a folder in Misc**, not a picker: a vertical list of 3 full-name rows
  (Normal Link, Zora, Fierce Deity - in that order) that each apply directly, rather than opening
  a separate picker screen. Zora and Fierce Deity now show their real mask icons (The Spriters
  Resource, Colbydude's Item Icons sheet, rows 20/21 - the same sheet already in the project);
  Normal Link keeps the hand-drawn mask placeholder since it has no mask to show.
- **"B Button Item" is now its own folder in Inventory** (matching Bottles' treatment) instead of
  an inline picker row. Dropped the "Fierce Deity Mask" option from it - it duplicated Misc's
  Play as... Fierce Deity, and its name didn't fit the picker grid without truncating; Gilded
  Sword and Great Fairy Sword (both real sprites) now fill the 2-column grid cleanly.
- **Removed "Examples" from HOME** - it was the template's own placeholder folder, not part of
  the actual cheat set.

### Picker UI overhaul, Bottles grid, Teleport filter fix, Fishing Hole Pass (builds 31–32)
- **Picker screen redesigned** (Bottle #1-7, B Button Item, Play as...): removed the big
  right-side "value preview" card (a hex-value box over generic art - leftover template
  boilerplate) and replaced the whole screen with a 2-column grid, one real icon per option, a
  small green dot marking the currently-set value. Every picker now fits on one screen with no
  scrolling (max option count is 18, and 2 cols x 9 rows = 18 exactly).
- **Real per-option art for the Bottle picker**: all 18 bottle contents (Red/Green/Blue Potion,
  Fairy, Deku Princess, Milk, Fish, Bug, Big Poe, Spring/Hot Spring Water, Gold Dust, Magic
  Mushroom, Sea Horse, Chateau Romani, Mystery/Mouldy Milk) now show their actual bottle icon
  from the sheet's own Bottles row, not a color swatch. Mystery Milk and Mouldy Milk share one
  icon (the sheet has no distinct art for those two). The row itself (in the Bottles folder list)
  now also shows the bottle's CURRENT content icon instead of a fixed generic one.
- **Bottles is now its own folder** (Inventory -> Bottles), laid out as a 2-column grid like
  HOME/Teleport, instead of 7 rows crammed into the Inventory list.
- **Teleport, filtered by Overworld/Dungeons, no longer wastes half the screen**: the 2-column
  grid forces a full-width break at every category header, and filtering left most categories
  with only one surviving destination - one item alone in a half-width column, column two wasted,
  a lot of scrolling for very little content, and long dungeon names clipped to fit that half
  width even though the other half sat empty. Fixed by only using the 2-col grid for the
  unfiltered "All" view; Overworld/Dungeons now render as a single full-width column, so every
  name shows in full and there's no more wasted space.
- **"Fishing Hole Pass"**: CONFIRMED on hardware (previously listed as an experimental "TEST:
  Fishing Tickets = 99" test cheat). Renamed and moved from Misc to Quest to match what it
  actually grants - the free rod loan pass, not a ticket count.

### Icon pass on the newest rows (build 30)
- **Bottle #1-7 pickers** now show a real sprite (the plain glass Bottle icon, cell `8,0` on the
  Item Icons sheet) instead of the generic Hex-Editor-style grid icon.
- **B Button Item picker** now uses the Gilded Sword icon (already in the sheet) as a stand-in -
  none of the three actual B-button items (Fierce Deity Mask, Gilded Sword, Great Fairy Sword)
  had all been individually placed on the sheet, so a themed "equip slot" icon was used instead
  of guessing which face/hilt cell was which.
- **Play as... picker** gets a new hand-drawn mask icon (round face, two eye holes, a mouth line)
  rather than a sheet sprite - the sheet's Masks section (rows 18-21) couldn't be confidently
  matched to Deku/Goron/Zora/Fierce Deity specifically without risking mislabeled art.
- **Items (Max/Inf ammo) folder** (Inventory) now uses the crossed-Arrows icon as its folder icon,
  closing the last folder row still on the generic engine vector icon (besides Tools/Settings,
  which stay generic on purpose).

### Forms, item pickers, Teleport grid fixes (builds 22–29)
- **Play as... picker** (Misc) — CONFIRMED. Found via a community "PLAY AS" field (0x7761FE)
  referenced by two independent AR code lists (an EU hold-R code confirms 0x00 = Fierce Deity)
  that didn't label the other values; took three rounds of hardware testing to land on what
  actually holds. The change only takes effect on the *next area transition* (door/warp/load),
  never instantly. Final picker: **Fierce Deity** (0x00, and it bypasses the vanilla game's
  boss-arena-only restriction entirely), **Zora** (0x02), and **Normal Link** (0x04) to revert.
  **Goron (0x01) and Deku Link (0x03) are left out** - both were tried fresh (not chained off
  another form) and the mask "comes off" on its own moments after loading, reverting to Link;
  whatever those two forms need beyond this one byte isn't covered by it. An earlier round had
  also seen a broken attack-animation loop when chaining a form request directly off Fierce
  Deity, but that turned out to be a symptom of the same Goron/Deku issue, not a general
  Fierce-Deity-transition problem - Zora chains fine.
- **Bottle #1-7 pickers** (Inventory) — set what each bottle holds (17 options: potions, fairy,
  milk, fish, bug, Big Poe, spring/hot spring water, gold dust, magic mushroom, sea horse,
  Chateau Romani, mystery/mouldy milk). Slot addresses were already confirmed; the content
  values come from two independent community AR-code lists that agree with each other, not
  individually hardware-tested.
- **B Button Item picker** (Inventory) — Fierce Deity Mask / Gilded Sword / Great Fairy Sword.
  Address (0x77632A) was identified back in the Phase 1/2 save-file analysis but never turned
  into an actual cheat until now.
- **Teleport grid navigation fix**: the 2-column grid rendering was already generic engine code
  (shared with HOME), but the actual D-Pad input dispatch was hardcoded to HOME only - Teleport
  was silently falling back to list-style navigation (Down selected sideways, Left/Right paged).
  Fixed by extending the same grid-nav dispatch to Teleport.
- **HOME broke as a side effect of adding the Teleport folder row**: the extra row pushed HOME's
  content past what fits in one screen, and HOME's scroll was hardcoded to always be 0 (an
  assumption that Time/Battle/Inventory/Quest/Misc/Examples/Tools/Settings would always fit).
  Fixed by extending the same pixel-offset scroll-follow logic Teleport uses to HOME too.

### Teleport (builds 16–21) — Phase 6
- **25 warp destinations** across Clock Town, Swamp, Mountain, Great Bay, Ikana and Ranch/Moon.
  The mechanism is CONFIRMED on hardware (Termina Field individually verified); the other 24
  reuse the same recipe with entrance indices from `PhlexPlexico/mm3d-practice-tools`' table,
  not each individually tested.
- **The path there was a real debugging story.** GlobalContext (the live, not-save game state
  struct) is **heap-allocated** - its address changes every boot/session. Three attempts
  (`next_entrance` write, a fuller write matching OoT's own proven recipe, and a completely
  different mechanism via `ocarina_state`) all did nothing, because each hardcoded an address
  found in an *earlier* session that had already gone stale by the time it was tested.
- **The fix**: `0x08363784` — the same stable pointer Moon Jump already uses for the player
  actor — turned out to point (almost) directly at GlobalContext too. Reading it fresh every
  call, the same way Moon Jump already does, is what made the warp actually fire.
- **Also found and fixed a 2-byte transcription error**: the `scene` field's offset was recorded
  as `0x14A` from an AI-summarized reading of the reference source; cross-checking a Cheat
  Search-found address against `GCTX_base + 0x148` proved the real offset is `0x148`. That
  2-byte error had thrown off every previous derived address in the same direction.
- **Method note**: found via Cheat Search Known Value narrowing on the `scene` field (2 bytes) —
  first with South Clock Town (`0x6F`) → Termina Field (`0x2D`), which converged to a single
  candidate but changed address between sessions (proving it wasn't reliable alone); then
  re-derived via the stable player-actor pointer instead, which is what actually held up.

### Button glyphs (build 19)
- Swapped the template's procedurally-drawn A/B/X/Y/L/R/D-Pad icons for OcarinaCTRComposer's own
  (ripped by **manpaint**, The Spriters Resource) - real 3DS button art instead of flat vector
  shapes. These are generic console button icons, not tied to either game, so reusing the file
  as-is was a straight drop-in.

### More icons: Tools, Settings, Moon Jump (build 15)
- **Tools folder + its 4 entries** now use sprites instead of generic engine vectors: the
  folder itself and Cheat Search both use the Lens of Truth, RAM Dumper uses the Dungeon Map
  (playful - it "maps" memory), Hex Editor uses the Compass (playful - it "pinpoints" a byte),
  and About uses a cropped Majora's Mask HOME-icon badge (same banner sheet as the logo).
- **Settings entries**: Change Theme uses Garo's Mask (playful - a mask changes your look, like
  a theme changes the menu's), Language uses a Trade Quest scroll. The Settings folder icon and
  the pure config toggles (notifications, autofill, hotkey rebinds) were left on the engine's
  generic vector icons - no game item maps onto "toggle a boolean" without forcing it.
- **Moon Jump** finally has an icon. There's no boot/footwear icon on this sheet (MM3D doesn't
  have swappable footwear the way OoT3D does), so this used the sheet's own best-effort read of
  an upward/reaching item near the Lens of Truth - flagged as tentative in code, since the
  sheet doesn't label it and it could turn out to be something else on closer inspection.

### Art and identity (builds 11–14) — Phase 4
- **21 real sprite icons** — Item Icons sheet (Colbydude) and UI sheet (xAct), both from The
  Spriters Resource. 17 next to cheat rows in Battle, Inventory, Items and Quest; 4 more added
  in build 14 as **HOME folder icons** (Time → Ocarina, Battle → Gilded Sword, Inventory → a
  Rupee Wallet, Quest → the Bombers' Notebook, Misc → the Pictograph Box). Reassigning the
  Ocarina and Notebook from cheat rows to folder icons freed up two cheats for new icons: Have
  all Items now uses a Trade Quest deed/scroll, All Bosses and Songs uses Odolwa's Remains.
  Cheats with no literal in-game icon kept their playful stand-in (bottled Fairy for Refill
  Magic, Hero's Shield for Enhanced Defense).
- **"Termina" theme**, revised in build 14: swapped to the `_02` background variants (top is now
  the sepia stone-carved version; bottom stays the color stained-glass version), and the text
  colors now match OcarinaCTRComposer's own "Zelda Classic" theme exactly, so the two sibling
  plugins read consistently. Original artwork by **samaBR**.
- **About screen logo** — the MM3D title wordmark, cropped from the "HOME Menu Icons and
  Banners" sheet (Hiccup / Cheesy Mac n Cheese, The Spriters Resource) with a stray 1px divider
  line matted out, drawn above the credits list the same way OcarinaCTRComposer does it.
- Not yet confirmed on hardware whether menu text stays legible over the art in every screen —
  flagged for a hardware check.

### Misc / Time Scrub (builds 9–10)
- **Moon Jump** (hold {L}+{A}) — the first **base+offset** cheat in this plugin: a pointer
  chain (`0x08363784` → `+0x4AC` → player actor) taken straight from the AR code's own decode,
  never independently re-derived, both levels null-checked. **Confirmed working perfectly** —
  the riskiest cheat so far turned out to be exactly right.
- **Time Scrub** (hold {R} default + D-Pad) — confirmed; forward is fully reliable. Going
  backward inherits the same known limitation as Set Time (crossing midnight advances the day).
- **Day Scrub** (hold {L} default + D-Pad) — confirmed; forward is reliable. Backward is
  unreliable around the area-transition boundary that gates when a day change actually shows —
  consistent with Set Day's known limitation, not a new bug. Forward is the recommended direction
  for both scrubs.
- Repurposed the template's two example hotkey slots (`hk1`/`hk2`, Settings) for these instead of
  adding new ones — they were wired to inert example code.

### Quest / Items (build 8)
- **Have all Masks** (one-shot) — fills all 24 mask slots. Decoded from the AR code's loop
  opcode (`D5`/`D3`/`C0`/`D8`/`D4`/`D1`/`D2`); the kickoff brief's own guess at this decode
  ("fill with 0x01") was wrong — it's an incrementing id sequence `0x32`-`0x49`, confirmed
  against a real 100%-complete save before ever touching hardware.
- **Have all Items** (one-shot) — fills the 16 main item slots, byte-for-byte matching a real
  100% save's item array (including which two slots stay empty).
- **All Bosses and Songs** (one-shot) — uses `0xCF 0xF7 0xCF`, the values read from a real 100%
  save. The AR code's own `0xFF 0xFF 0xFF` does not match any legitimate save state.
- **All Stray Fairies** (one-shot) — fills all four dungeon bytes; the AR code's own version
  (a single 16-bit write) only ever filled one dungeon.
- **Max/Inf Arrows, Bombs, Bombchus, Deku Sticks, Deku Nuts, Magic Beans, Powder Keg** (toggles)
  — ported directly from the AR code's per-item addresses.

### Battle / Inventory (builds 6–7)
- **Refill Hearts** (one-shot) — fills current health to your real capacity.
- **Max Hearts** (toggle) — holds capacity + current at 20 hearts while on; **turning it off
  restores your real values**, captured the moment the toggle turned on (not a guess).
- **Refill Magic** (one-shot) — fills the magic meter to your real cap (normal or Double Magic).
- **Enhanced Defense** (toggle) — halves damage taken while on; same capture/restore-on-off
  behavior as Max Hearts. Moved from Inventory to Battle to match the folder convention.
- **Max Rupees (999)** (toggle) — the first cheat found live on hardware via Cheat Search
  (Known Value 27 → 57, poked `0x776318`, in-game counter matched). Confirms the AR code list's
  own address, which the offline save-file analysis alone could not settle.
- **Gilded Sword + Mirror Shield**, **Large Quiver + Big Bomb Bag** (one-shot) — confirmed.

### Time (builds 3–4)
- **Set to Day 1/2/3** (one-shot) — confirmed, but only takes effect on your next area
  transition (door, warp, load), not instantly. Likely a live/cached copy of the day that's
  separate from the save-struct byte we write; not investigated further.
- **Set Time to 6AM/10AM/6PM** (one-shot) — confirmed, applies instantly. **Known limitation:**
  jumping to an earlier time than the current one advances the day (the game reads the decrease
  as midnight passing). Tried re-asserting the day byte in the same write; did not help, so
  whatever the game actually compares isn't that byte. Left as a known limitation.

### Scaffolding (build 1)
- Repo scaffolded from the [CTRComposer](https://github.com/samaBR85/CTRComposer) blank
  template, mirroring the [OcarinaCTRComposer](https://github.com/samaBR85/OcarinaCTRComposer)
  layout: `RawPlugin/` (source), `Tools/` (art pipeline scripts), `SaveGames/` and `references/`
  (dev-only, gitignored).
- `PLUGIN_DIR` set to `/luma/plugins/0004000000125500/`; `plgInfo` targets Title ID `0x00125500`.
- Offline decode of `references/mm3d-ar-cheats-usa-0004000000125500.txt` against the 22-save
  progressive save corpus (`SaveGames/`) — cross-checked address map for most of the AR cheat
  list, including the three AR entries whose semantics weren't otherwise documented
  (`[Refill Hearts (Current)]`, `[Have all masks]`, `[Fill Magic (Current Cap)]`).
