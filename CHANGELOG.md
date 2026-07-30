# Changelog — MajoraCTRComposer (Zelda: Majora's Mask 3D)

A self-rendered `.3gx` overlay plugin for The Legend of Zelda: Majora's Mask 3D
(New 3DS · **USA**, Title ID `0004000000125500`, version **1.1.0**). Version numbers follow
SemVer; the **build** counter is the running iteration count shown on-screen (`bNN`).

---

## Unreleased · builds 1–21

Boots and menu confirmed on real MM3D v1.1.0 hardware at build 1; every cheat below was
individually confirmed on the same console unless noted otherwise.

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
