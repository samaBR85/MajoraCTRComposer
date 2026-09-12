# MajoraCTRComposer

**A cheats + tools overlay for *The Legend of Zelda: Majora's Mask 3D* — built on
[CTRComposer](https://github.com/samaBR85/CTRComposer), a raw `.3gx` overlay engine for the 3DS.**

Press **SELECT** in-game to open a themed menu with cheats, a Cheat Search, a RAM Dumper, and a
Hex Editor — all rendered by the plugin itself.

Work in progress — see [`RawPlugin/README.md`](RawPlugin/README.md) for build instructions.

---

## Status

This build targets **Majora's Mask 3D, USA, v1.1.0** (Title ID `0004000000125500`). Boots and
menu confirmed on real hardware. The cheat table is hardware-confirmed across Time, Battle,
Inventory, Items (ammo), Quest and Misc (Moon Jump, Play as... forms), plus a Teleport with
location filters and item pickers (bottle contents, B button item). A 9-category 100% Checklist
and an SD-loaded Game Guide round out the build — see `CHANGELOG.md` for the full list and known
limitations.

## Roadmap

1. **Skeleton and boot baseline** — done.
2. **First verified cheats** — done.
3. **Rest of the cheat table** — done, minus the minigame code patches (moved to phase 8).
4. **Art and identity** — done. Item/UI sprite sheets (Colbydude, xAct), button glyphs
   (manpaint), the "Termina" theme (original art by samaBR), an About-screen logo, and sprite
   icons throughout HOME, Tools and Settings, all from The Spriters Resource / original art.
5. **100% Checklist** — done (first-pass content complete). A 9-category, 194-item tracker:
   Masks (24), Heart Pieces (52), Songs (13), Bosses (4), Stray Fairies (5), Bomber's Notebook
   (63), Owl Statues (10), Bottles (7), Equipment (16). ~49 items auto-fill from the save-file
   map (Masks and Stray Fairies auto-fill confirmed on hardware); the rest are manual toggles,
   each with a Hint and a `{X}` "Where" reveal. Open gaps: most Bomber's Notebook events and
   the Owl Statues completion state have no mapped save bit yet, so they stay manual until a
   Cheat Search pass finds them.
6. **Teleport** — done. Destinations with folder filters (All / Dungeons / Overworld / Owl
   Statues); the warp mechanism (GlobalContext, heap-allocated, resolved via the same stable
   pointer Moon Jump uses) is confirmed on hardware.
7. **Game Guide** — done. A progression-ordered walkthrough loaded from the SD card
   (`guide/<Language>/game.txt`), with a short embedded English fallback if the file is missing.
   English only for now; drop in other languages without recompiling. Credits live on the About
   screen.
8. **Minigame code patches** — done. A **Minigames** folder with five instruction-patch cheats
   (Town/Swamp Shooting Galleries, Beaver Swimming, Boat & Jump, Auto-win Honey & Darling). The
   Town Gallery is hardware-confirmed. `.text` is read-only to user code on this console, so the
   store runs in supervisor mode via `svcCustomBackdoor` (the RWX flip leaves `.text` RW for
   privileged / RO for user) - the same privilege the Rosalina cheat engine uses. Never
   auto-enabled on boot. `Easy Deku Rupee Game` is a plain data write, not a code patch, so it is
   not part of this set.

## Requirements

- A 3DS (New 3DS recommended) running **Luma3DS** with the **plugin loader** enabled.
- A copy of **Majora's Mask 3D** — USA v1.1.0. Other regions/versions are untested; addresses
  are known to shift between v1.0 and v1.1 (see `references/`).

## Install

```
sdmc:/luma/plugins/0004000000125500/MajoraCTRComposer.3gx
```
Keep only one `.3gx` in that folder. Open Rosalina (`L + Down + SELECT`) → Plugin Loader →
Enabled, launch Majora's Mask 3D, press SELECT.

## Build from source

Requires **devkitPro** (devkitARM + libctru) and **3gxtool 1.3**
(`thepixellizeross-win/3gxtool`).

> The devkitPro toolchain breaks on paths containing spaces. Build from a space-free path,
> using the devkitPro msys2 shell:

```bash
/c/devkitPro/msys2/usr/bin/bash.exe -lc 'cd <project-no-spaces> && make'
```

### Make your own plugin

The engine behind this plugin — **[CTRComposer](https://github.com/samaBR85/CTRComposer)** — is
game-agnostic and released separately as a blank, buildable template. See that repo to bootstrap
a plugin for another 3DS game.

## Credits

- **JourneyOver** — `CTRPF-AR-CHEAT-CODES`, source of the MM3D cheat list this build ports from
- **HTW (HelpTheWretched)** — progressive save-file collection used to map save data to RAM
- **PhlexPlexico** — `mm3d-practice-tools`, referenced for struct/field context where used
- **Colbydude** — MM3D Item Icons sheet, The Spriters Resource
- **xAct** — MM3D UI sheet (rupee icon), The Spriters Resource
- **Hiccup / Cheesy Mac n Cheese** — MM3D HOME banner (About screen logo), The Spriters Resource
- **manpaint** — 3DS button glyphs (A/B/X/Y/L/R/D-Pad), The Spriters Resource, reused from OcarinaCTRComposer
- **Zelda Wiki (zeldawiki.wiki)** — MM3D Owl Statue icon and 3D-render model, and 22 of the 24 individual Mask icons, used in the 100% Checklist
- **zeldaret/mm** — the Majora's Mask N64 decompilation project, referenced for item id/name ground truth (matches our own confirmed in-game values)
- **samaBR** — Termina theme background art (original artwork)
- **Luma3DS** (plugin loader) — https://github.com/LumaTeam/Luma3DS
- **3GX plugin loader / 3gxtool** — PabloMK7 — https://github.com/PabloMK7

*Rendering techniques referenced from CTRPluginFramework.*

---

**Made with [Claude](https://claude.ai).**

---

## License

The plugin's own source code is **[MIT](LICENSE)**. Third-party material (see **Credits**) stays
under its owners' rights, and game IP is © Nintendo — see the disclaimer below.

---

## Disclaimer

Fan project, **non-commercial**. *The Legend of Zelda: Majora's Mask* and all game content are
© Nintendo. This plugin contains no game assets; it reads and modifies the running game's memory
on the user's own console. Use at your own risk.
