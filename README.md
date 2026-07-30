# MajoraCTRComposer

**A cheats + tools overlay for *The Legend of Zelda: Majora's Mask 3D* — built on
[CTRComposer](https://github.com/samaBR85/CTRComposer), a raw `.3gx` overlay engine for the 3DS.**

Press **SELECT** in-game to open a themed menu with cheats, a Cheat Search, a RAM Dumper, and a
Hex Editor — all rendered by the plugin itself.

Work in progress — see [`RawPlugin/README.md`](RawPlugin/README.md) for build instructions.

---

## Status

This build targets **Majora's Mask 3D, USA, v1.1.0** (Title ID `0004000000125500`). Boots and
menu confirmed on real hardware. 22 cheats confirmed so far across Time, Battle, Inventory,
Items (ammo), Quest and Misc (including Moon Jump, the first base+offset cheat) — see
`CHANGELOG.md` for the full list and known limitations.

## Roadmap

1. **Skeleton and boot baseline** — done.
2. **First verified cheats** — done.
3. **Rest of the cheat table** — done, minus the minigame code patches (moved to phase 8).
4. **Art and identity** — done. Item/UI sprite sheets (Colbydude, xAct), the "Termina" theme
   (original art by samaBR), an About-screen logo, and sprite icons throughout HOME, Tools and
   Settings, all from The Spriters Resource / original art.
5. **100% Checklist** — auto-fill tractable from the save-file map already built; not started.
6. **Teleport** — depends on the player-actor pointer (confirmed via Moon Jump) plus MM3D's
   scene/entrance tables; not started.
7. **Game Guide** — needs an MM3D walkthrough text source; not started.
8. **Minigame code patches** — instruction-patch cheats (Shooting Galleries, Beaver Swimming,
   Honey & Darling). Highest risk in the plan (RWX + flush, never auto-enabled on boot);
   deliberately last. `Easy Deku Rupee Game` turned out to be a plain data write despite being
   grouped with these in the source list, so it'll ship as a normal cheat instead, whenever it's picked up.

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
