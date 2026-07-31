# Changelog — MajoraCTRComposer (Zelda: Majora's Mask 3D)

A self-rendered `.3gx` overlay plugin for The Legend of Zelda: Majora's Mask 3D
(New 3DS · **USA**, Title ID `0004000000125500`, version **1.1.0**). Version numbers follow
SemVer; the **build** counter is the running iteration count shown on-screen (`bNN`).

---

## Unreleased · builds 1–39

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
