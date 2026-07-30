# Changelog — MajoraCTRComposer (Zelda: Majora's Mask 3D)

A self-rendered `.3gx` overlay plugin for The Legend of Zelda: Majora's Mask 3D
(New 3DS · **USA**, Title ID `0004000000125500`, version **1.1.0**). Version numbers follow
SemVer; the **build** counter is the running iteration count shown on-screen (`bNN`).

---

## Unreleased · builds 1–10

First hardware-confirmed cheats. Boots and menu confirmed on real MM3D v1.1.0 hardware at
build 1; every cheat below was individually confirmed on the same console.

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
