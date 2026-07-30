# MajoraCTRComposer — source

Source for the **Majora's Mask 3D** build of **CTRComposer**, a raw `.3gx` overlay engine
for the 3DS (Luma3DS plugin loader). CTRComposer renders its own UI to the framebuffer and
runs inside the game's process, so it stays self-contained across games and Luma versions.

For the engine architecture and how to build a plugin for another game, see
[`../CTRComposer-Blank-Template.md`](../CTRComposer-Blank-Template.md) and the upstream
[CTRComposer](https://github.com/samaBR85/CTRComposer) repo (`README-template.md` in this
folder is its own README, kept for reference).

## Build

The devkitPro toolchain breaks on paths with spaces — copy `RawPlugin/` to a space-free path
and run `make` in the devkitPro msys2 shell:
`/c/devkitPro/msys2/usr/bin/bash.exe -lc 'cd <path> && make'`.

Requirements: devkitARM + libctru (pacman) and `3gxtool` 1.3 (`thepixellizeross-win/3gxtool`).

Install on the SD card: `luma/plugins/0004000000125500/MajoraCTRComposer.3gx` (USA Title ID;
keep only the one `.3gx` in that folder).

## Credits

- **JourneyOver** — `CTRPF-AR-CHEAT-CODES`, source of the MM3D cheat list this build ports from
- **HTW (HelpTheWretched)** — progressive save-file collection, used to map save data to RAM
  addresses (offline validation of the cheat table)
- **PhlexPlexico** — `mm3d-practice-tools`, referenced for struct/field context where used
- **CTRPluginFramework** — rendering techniques (system font, thread pause, LCD) referenced from it
- **Luma3DS** (plugin loader) — https://github.com/LumaTeam/Luma3DS · **PabloMK7** — https://github.com/PabloMK7
- Game assets belong to **Nintendo**; fan project, non-commercial.

## License

Personal / educational use. Not for commercial redistribution.
