# CTRComposer

**A raw `.3gx` overlay engine for the 3DS, and a buildable starting point for a cheats/overlay
plugin for *any* 3DS game.**

`devkitARM + libctru` · `Luma3DS PLGLDR` · `3gxtool 1.3` · any Title ID

This repository builds **two plugins from the same tree**. They are the same engine — the
difference is who is holding it.

## Which one do you want?

### 🔧 `CTRComposer-BlankTemplate.3gx` — *build a plugin for one game*

**For developers.** You will edit C, and in exchange you get the whole engine for free: folder
menus, live themes, an on-screen keypad, inline button glyphs, SD persistence, localization, a
favourites quick menu, a guide reader, a progress tracker, and the four memory tools.

What you add is the part no engine can give you: **your game's addresses**. Everything else is
already written.

> Installs to `luma/plugins/<TitleID>/` — one `.3gx` per game folder.
>
> Ships with **zero game art and zero game addresses**. The example cheats are inert placeholders
> that write nothing until you set `EXAMPLE_ENABLED` in [`Sources/main.c`](Sources/main.c).

**Pick this if:** you want a cheat menu for a specific game · you're reviving an old `.plg`
that no longer loads · you want an overlay UI for a homebrew idea of your own.

### 🔎 `default.3gx` — *memory tools, in every game, no code*

**For players and explorers.** Drop one file on the SD card and every title on the system gets
**Cheat Search**, a **RAM Dumper** and a **Hex Editor** on SELECT. Nothing to compile, nothing
to configure, no Title ID to look up.

It carries **no cheats**, and that is a fact about reality rather than a missing feature: a
cheat is a memory address, and an address belongs to one game *and one region*. What is genuinely
universal is the tooling — searching memory, reading it, editing it — so that is what it carries.

> Installs to `luma/plugins/default.3gx` — the root, not a game folder.

**Pick this if:** you want to poke at any game without writing code · you're hunting addresses
*before* deciding to build a plugin · you want a permanent hex editor on the console.

### Using both

They compose, and the rule is simple: **a per-game plugin always wins.** Luma only falls back to
`default.3gx` for titles that have no folder of their own. So your own plugin runs where you
built one, and the toolkit covers everything else.

The natural workflow is exactly that order — hunt addresses anywhere with `default.3gx`, then
build the real plugin from the template once you have them.

<sub>Under the hood the second build is just `TOOLS_ONLY` set to `1` in `Sources/main.c`. The
per-game furniture — example cheats, tracker, game guide — is compiled out along with every menu
string that referenced it, which is checked by CI.</sub>

**[Project page](https://samabr85.github.io/CTRComposer/)**

**[Download the latest release](https://github.com/samaBR85/CTRComposer/releases/latest)** —
both `.3gx` files are attached to every tagged release. Or build them yourself: see *Quick
start* below.

<p align="center">
  <img src="screenshots/hero.png" alt="CTRComposer running on a 3DS: the HOME menu on the top screen and the control legend on the bottom" width="360">
</p>

Everything below is a real capture of this template running on hardware, with no game data of
its own — the game behind the window is just whatever title it was loaded into.

| | |
|:--:|:--:|
| <img src="screenshots/cheat_examples_01.png" width="300"><br>**Example cheats** — toggles get a checkbox, one-shot actions get a plain box. All inert until you add addresses. | <img src="screenshots/cheat_examples_02.png" width="300"><br>**Value picker** — for cheats where a toggle makes no sense: pick from a list, with a live preview of the current value. |
| <img src="screenshots/cheat_search.png" width="300"><br>**Cheat Search** — known/unknown value, scan types, region filter, one-step undo. | <img src="screenshots/hex_editor.png" width="300"><br>**Hex Editor** + the code-drawn keypad. A–F grey out in DEC mode; read-only memory is refused, not crashed. |
| <img src="screenshots/checklist_01.png" width="300"><br>**Tracker** — per-item progress that syncs from memory. Ships with placeholder rows only. | <img src="screenshots/checklist_02.png" width="300"><br>**Tracker detail** — status, hint, and a location you reveal with `X`, so a guide can stay spoiler-free. |
| <img src="screenshots/game_guide.png" width="300"><br>**Guide reader** — word-wrap, categories, and resume. Load pages from C or from the SD card. | <img src="screenshots/settings.png" width="300"><br>**Settings** — theme, language, toast, and rebindable in-game hotkeys, all persisted. |
| <img src="screenshots/about.png" width="300"><br>**About** — text only. No logo image, so nothing third-party to inherit. | |

## Quick start

```sh
# 1. Build (devkitPro msys2 shell, from a path with NO SPACES in it)
/c/devkitPro/msys2/usr/bin/bash.exe -lc 'cd /c/your/checkout && make'

# 2. Install: one .3gx per plugin folder, named for your game's Title ID
cp CTRComposer-BlankTemplate.3gx  <SD>/luma/plugins/<TitleID>/
```

Then launch the game with Luma's plugin loader enabled and press **SELECT**.

The plugin **loads** under any Title ID — `Targets:` is empty in the `.plgInfo`. Where it
**stores** its data is one `#define` in [`Sources/main.c`](Sources/main.c):

```c
#define PLUGIN_DIR "/luma/plugins/0004000000033500/"   // your game's folder
```

Left empty, `Settings.cfg`, `Favorites.txt`, `Tracker.txt`, `lang/`, `guide/` and `dumps/`
land in `/luma/plugins/` itself. That works for a first run, but the folder is shared by every
game — set it before you ship.

## What's in the box

| Path | What it is |
|---|---|
| `Sources/main.c` | The whole engine. The only game-specific part is the clearly-marked cheat section. |
| `Sources/sysfont.c` | 3DS shared-font renderer (APT IPC, no `aptInit`). |
| `Sources/bootloader.s`, `csvc.s` | Entry stub and the `svcControlProcess` wrapper. |
| `Includes/glyphs.h` | Button glyphs — **generated**, see `Assets/gen_glyphs.py`. |
| `Includes/themes.h` | Theme table. Ships one neutral monochrome theme. |
| `Includes/guide.h` | Embedded guide pages (generic placeholders). |
| `Assets/gen_glyphs.py` | Redraws the button-glyph sheet from primitives. |
| `.github/workflows/build.yml` | CI: builds the `.3gx` on every push and uploads it as a run artifact. |

---

## What CTRComposer is

A C plugin that renders its own UI directly to the framebuffer: a themeable window, the system
font, folder menus, tools and themes. It runs **inside the game's process** (same address
space → writing memory is just a pointer) and draws everything itself, so a plugin is
**self-contained and portable across games and Luma3DS versions**.

CTRComposer grew out of — and owes a great deal to — **CTRPluginFramework** and the earlier
community `.plg` plugins. Those projects made 3DS plugins possible and are the direct
inspiration here. CTRComposer is meant as a **framework to build new plugins from scratch, or
to revive older `.plg`/`.3gx` plugins** that no longer load on current Luma3DS builds — giving
that work a fresh, self-rendered foundation that doesn't depend on any particular framework
build matching the game.

**About 90% of the code is game-independent.** A new plugin is essentially a **cheat table +
art + Title ID**. Everything else (rendering, font, pause, persistence, menu, themes, tools,
guide) is reused as-is.

**Engine capabilities already built in — all optional, all yours to configure:**
- A **folder menu primitive** (`Item` / `Folder`, section headers, category icons) with layout
  left to the creator: a single-column list, a multi-column grid, groups with separators —
  whatever fits the plugin. The reference build happens to use a 2-column HOME grid, but that's
  one choice among many, not a rule of the engine.
- A **live theme-switch mechanism** (recolor every menu instantly, no call-site changes — see
  section 7), with **auto-contrast** so text stays readable on both light- and dark-background
  themes. The template ships with **one neutral, monochrome starter theme** (near-black
  background, white/light-grey text, no colored accent — the plain CTRPluginFramework-style look,
  not any specific game's palette) so a fresh plugin doesn't inherit a Zelda color scheme by
  default. Add as many themes as you want, or none at all.
- A **sprite system**: RGBA4444 icon sprites with alpha, a **nearest-neighbour scaled blit** for
  art of any size, hand-drawn vector icons as an art-free alternative, and **inline button-glyph
  tokens** (`{A}`/`{B}`/`{D-Pad}`…) that render as icons inside any label or hint (see section 4).
- Tools: **Cheat Search** (known/unknown + undo + regions), **RAM Dumper**, **Hex Editor**, **About**
- A **guide reader** (scrollable embedded text, word-wrap + resume) you can point at *your*
  game's content — or skip entirely if the plugin doesn't need one. A **Plugin Guide** (how-to-use
  pages for the plugin itself) is the same reader with generic, game-agnostic content; write your
  own pages instead of copying game-specific ones from another build.
- A **UI localization mechanism** (`T("English source")` → looked up in an SD text file per
  language, English embedded as the always-available fallback — see section 7). The template
  ships **English-only**; add language files only if the plugin wants more than one, in whichever
  languages the creator picks.
- Toast notifications; a **favorites quick menu** (favourite cheats, *and* folder / tool shortcuts);
  an on-screen numeric keypad (skinnable with sprites); **auto-repeat** (hold a D-pad direction to
  keep scrolling); **rebindable in-game hotkeys**; SD-card persistence with config migration.
- **SELECT** = return to the game from any screen; reopening resumes exactly where you left off (even inside a tool)

---

## Design in one paragraph

Because CTRComposer draws its own UI and uses **no game hooks**, the same engine runs unchanged
across titles and Luma versions — there are no fixed hook-wrapper addresses or per-game memory
assumptions to line up. That portability is the whole point: write the engine once, and each
new game only needs its own cheats, art and Title ID.

---

## 1 · Toolchain and skeleton

- **devkitARM + libctru** and **3gxtool 1.3** (pacman `thepixellizeross-win/3gxtool`, which
  writes the `3GX$0002` container Luma expects — the older `3GX$0001` is rejected).
- Link at `0x07000100`. In `plgInfo`, `MemorySize` accepts `2MiB / 5MiB / 10MiB` (use 5MiB if
  you embed a large guide or snapshots). Build rule: `3gxtool -s $(elf) $(plgInfo) $@`. Flag `-D__3DS__`.

> **Gotcha — spaces in paths** — The devkitPro toolchain breaks on paths containing spaces
> (`make[1]: /d/My Project: Is a directory`). Build from a **space-free path**, and run make
> through the **devkitPro msys2 shell**, not git-bash:
> `/c/devkitPro/msys2/usr/bin/bash.exe -lc 'cd <project> && make'`. Also keep **one `.3gx` per
> plugin folder**.

Luma injects a `PluginHeader` at `0x07000000`. The skeleton sets up the heap and starts a
worker thread:

```c
void main(void) {
    PluginHeader *h = (PluginHeader *)0x07000000;
    if (h->magic != HeaderMagic) return;
    __ctru_heap = h->heapVA;  fake_heap_start = (char*)h->heapVA;
    fake_heap_end = fake_heap_start + h->heapSize;
    srvInit();  plgLdrInit();
    svcCreateThread(&t, ThreadMain, 0, stackTop, 30, -1);
}
void *__service_ptr = NULL;   // the 3dsx crt0 defines this; a plugin must declare it, or srvGetServiceHandle won't link
```

> **Gotcha — ThreadVars** — A thread created with a raw `svcCreateThread` has no libctru
> `ThreadVars`, so malloc/printf/hid/fs will `svcBreak` in `__syscall_getreent`. **At the start
> of `ThreadMain`, before any newlib/hid/fs call**, seed the TLS:
> ```c
> u32 *tv = (u32*)getThreadLocalStorage();
> tv[0] = 0x21545624;            // THREADVARS_MAGIC (@0)
> tv[2] = (u32)_impure_ptr;      // reent (@0x8)
> tv[4] = 0;                     // fs_magic (@0x10) = use the global fs session
> ```
> Give the thread a stack of at least 16KB (printf + hid + fs run deep).

---

## 2 · Reading input

Buttons come straight from a hardware register — no hid service needed.

```c
#define REG32(a)  (*(volatile u32*)((a) | (1u<<31)))     // uncached physical mirror
#define HID_PAD   (REG32(0x10146000) ^ 0xFFF)            // active-low → XOR
u32 down = pad & ~prev;   prev = pad;                    // own edge-detect
```

**Auto-repeat (typematic).** Since you own the edge-detect, a tiny helper turns "held" into a
repeating `down` for navigation keys: fire on the initial press, then again every N frames after an
initial delay (track a per-direction hold counter). Wrap `ARepeat(pad, &prev, &hold)` around the
D-pad so holding a direction keeps the cursor moving; leave action buttons on plain edge-detect.

> **Gotcha — do not call `hidInit()`** — On the New 3DS, `hidInit()` internally calls
> `irrstInit()`, and `ir:rst` conflicts with the running game → it **freezes the game and locks
> the HOME button**. For the **touch screen** (on-screen keyboard) do a manual init that never
> touches ir:rst: `srvGetServiceHandle("hid:USER")` + command `0x000A0000` (GetIPCHandles;
> reply[3] = shmem, [4..8] = events) + `mappableInit(OS_MAP_AREA_BEGIN, OS_MAP_AREA_END)` +
> `mappableAlloc(0x2B0)` + `svcMapMemoryBlock`. Read touch from the shmem: `id = shmem[42+4]`
> (cap 7), position packed in `shmem[42+8+id*2]` (px = low16, py = high16), valid at `[+1]`.

> **Gotcha — ZL / ZR** — Only the 12 classic buttons are in that register. **ZL/ZR (New 3DS)
> come from `ir:rst`**, and games without Circle Pad Pro support have **no `ir:rst` in their
> exheader ACL**, so `srvGetServiceHandle` fails. A `.3gx` can't patch the exheader. **Assume
> ZL/ZR are unreachable** from a pure plugin — use L/R combos instead.

---

## 3 · Applying cheats  ← *the only game-specific part*

The plugin shares the game's address space, so writing memory is a pointer. Only the addresses
change between games.

**Where addresses come from:** reuse an existing source (a `.plg`/`cheats.cpp`, an Action
Replay code bank, community plugins — each write becomes one line of C), **or** discover them
with the engine's own **Cheat Search**. They are **always region/version-specific** — re-anchor
when you change region.

> **Technique — save-diffing (mapping packed save-data fields).** Cheat Search finds a value by
> its *magnitude* — great for a rupee counter or a health word. It's awkward for **packed
> bitfields**, where many unrelated facts share one byte (which songs you know, which switches a
> dungeon has flipped, which optional collectibles are taken): you'd have to trigger each event
> live and diff RAM, and a byte that gains several bits at once is ambiguous. **Progressive save
> files** solve this cleanly, and many games have community *"start-to-finish"* save collections —
> a series of saves, each a small, *documented* step further into the game.
>
> The method: **(1) Anchor the save file to RAM.** Find a field in the `.bin` whose bytes you
> already know — e.g. an inventory array that matches a known "give all items" cheat buffer
> byte-for-byte. That single match pins the whole `file_offset ↔ RAM_address` mapping (they differ
> by a constant), because a save file is usually a raw serialization of the same in-memory struct,
> so **bit order is identical in the file and in RAM**. **(2) Diff consecutive saves.** For each
> step, the bits that flip on correspond to exactly what that save gained (a song, a stone, a
> flag). Line the diffs up against the collection's written progress notes and each bit gets a
> name. **(3) Cross-check.** The map is self-validating — if some bits were already confirmed
> working on hardware (say, medallion bits), the save-diff should reproduce them exactly; when it
> does, trust the rest it found the same way. A short throwaway script (Python over the raw files)
> does all three in one pass — no console round-trip needed to *derive* the map, only to
> spot-check it afterward with a deliberately *partial* save.
>
> Caveat: verify the anchor (a byte-for-byte match, not a lucky single byte), and watch for a
> file header/checksum shifting the absolute offset — the **relative** bit layout within a field
> is unaffected. Credit the save authors if you ship what their data helped you map.

```c
// Data cheats (most of them):
*(volatile u16*)<addr> = <value>;                       // direct write
u32 base = *(volatile u32*)<playerPtr>;                 // base+offset pattern
if (base) *(volatile u32*)(base + <offset>) = <value>;

// Code cheats (instruction patches in .text R-X) — the "how" is universal:
svcControlProcess(CUR_PROCESS_HANDLE, PROCESSOP_SET_MMU_TO_RWX, 0, 0); // once
// ALWAYS save the original instructions so the cheat can be turned off!
*(volatile u32*)<codeAddr> = <newInstruction>;
svcFlushEntireDataCache();  svcInvalidateEntireInstructionCache();
```

> Patching game code from the plugin is proven on hardware. **Never** auto-enable a code patch
> (invincibility, etc.) on boot.

> **Technique — driving the game's own actions (not just setting values).** Many effects that look
> like they'd need a function call are really just **memory writes that the game's own loop reads
> back**. Set the fields a routine consumes, then flip the flag that triggers it — the game does the
> work next frame, no hooks. Classic uses: a **warp / level-load** (write the destination/transition
> fields the load routine reads, then set its "load now" flag) or a **respawn / checkpoint restore**
> (populate the game's own respawn struct with a saved position, then raise the respawn flag). The
> fields come from the same sources as any address (a practice-menu, a decomp, Cheat Search). Guard
> it: only fire when a sanity read confirms you're in a valid state (e.g. a plausible current-level
> id and no transition already pending), or a bad write lands in the wrong place and crashes.

---

## 4 · Drawing the overlay

You talk to the LCD registers directly.

| Register | Offset | What it is |
|---|---|---|
| LCD top / bottom | `0x10400400` / `0x10400500` | base |
| FramebufferA1 / A2 | `+0x68` / `+0x6C` | the two buffers (double-buffering) |
| Format | `+0x70` | 0=RGBA8(4bpp) 1=RGB8(3) 3=RGB565(2) |
| Select | `+0x78` | bit0 = which buffer is on screen |
| Stride | `+0x90` | bytes per column |

The framebuffer is **column-major, Y-flipped**, bytes in **BGR** order, and the physical mirror
is **slow** (~18 ms for a full screen). Two techniques solve it:

- **RAM compose buffer** (RGB888): draw everything into a normal cached buffer (this enables
  real alpha-blending) and do **one** blit at the end.
- **Double-buffering:** draw into the *hidden* buffer and flip with `Select ^= 1`. No flicker.

```c
u32 off = x*stride + (H-1-y)*bpp;                       // column-major + Y-flip
volatile u8 *px = (u8*)((fb + off) | (1u<<31));
px[0]=b; px[1]=g; px[2]=r;                              // BGR
```

**The system font** (the visual key) — the same anti-aliased font is the 3DS shared font,
fetched by IPC to APT **without `aptInit`**:

```c
srvGetServiceHandle(&apt, "APT:U");                     // tries U -> A -> S
cmdbuf[0] = 0x00440000;                                 // GetSharedFont; reply[2]=addr [4]=handle
svcMapMemoryBlock(handle, 0, MEMPERM_READ, MEMPERM_DONTCARE);
CFNT_s *font = (CFNT_s*)(addr + 0x80);                  // sig 'CFNT'/'CFNU'
```

> **Gotcha — font mapping** — In the font's `svcMapMemoryBlock` the address **must be `0`** (the
> block carries its own fixed address). Passing the returned `addr` fails **silently**.

Backgrounds? Convert a BMP to a C header in **RGB565** (a Python script) and embed it in the
`.3gx` — no loose SD files. (For long text like a guide, embed it as C strings — see section 7.)

**Sprites (icons, keys, buttons).** Small art is embedded as **RGBA4444** arrays (`v = R4<<12 |
G4<<8 | B4<<4 | A4`), decoded per pixel and **alpha-blended** into the compose buffer (`a =
(v&0xF)*17`; `p = (p*(255-a) + c*a)/255`). Two blit paths cover everything: fixed-size icons drawn
1:1, and a **nearest-neighbour scaled blit** (`DrawScaled(dstX,dstY,dstW,dstH, px, srcW,srcH)`) that
maps any source size onto any rect — so one small source tile can fill a large key, and a single
routine handles every art size.

> **Gotcha — 8→4-bit packing brightens art.** When you pack an 8-bit PNG to 4-bit, truncating
> (`r>>4`) then decoding (`*17`) rounds most colours **upward** (up to +15/255), so the art reads
> lighter than the source — very visible on light tiles. Pack with **round-to-nearest**
> (`clamp((r+8)/17, 0, 15)`), the true inverse of the `*17` decode, to remove the bias.

**Vector icons** — an alternative to embedding art: draw an icon from primitives (filled rects /
lines) in code. Zero asset bytes, theme-recolourable, and crisp at any size — good for simple
glyphs (pins, arrows, meters).

**Inline button glyphs — a CTRComposer standard.** The engine ships a small sheet of the **3DS
button faces** (`{A}` `{B}` `{X}` `{Y}` `{L}` `{R}` `{D-Pad}`, and START/SELECT as short words) as
sprites, and the text routine scans any string for those tokens and blits the matching glyph in
place of the token. This is **the** way control legends, hints and labels are written throughout a
CTRComposer plugin — you never spell out "press the A button": you write the string with the token
and the real console icon renders. It is a first-class convention, not an add-on:
- Use it **everywhere** a control is named — the on-screen control legend / pause-help card, footer
  hints in every tool, and cheat labels/info that mention a button.
- Works in **both font sizes** (menu and the small 6×10), so the same token renders inline in a
  title or a caption.
- **Localises cleanly** — the token is not translated, so `"{A} select"` stays correct in every
  language; the glyph is drawn, only the surrounding words change.
- Pair it with **rebindable hotkeys** (section 7): store the mapped button as a token and the info
  card shows the *live* binding as its icon.

Because the glyph set is game-independent, it's part of the reusable engine — a new plugin gets the
console-accurate button legend for free.

---

## 5 · Freezing the game + toast

```c
// PROCESSOP_SCHEDULE_THREADS = 5. arg=1 pauses, 0 resumes.
svcControlProcess(CUR_PROCESS_HANDLE, 5, 1, (u32)ThreadPredicate);
bool ThreadPredicate(void *th) {                         // spare the plugin's own thread
    u32 tls = *(u32*)((u8*)th + 0x94);                   // KThread->tls
    if (*(void**)0xFFFF9000 != th && *(u32*)tls != 0x21545624) return true;
    return false;                                        // 0x21545624 = THREADVARS_MAGIC
}
```

> **Gotcha — toast without a frame hook** — The game erases your overlay every frame (~33 ms).
> Re-stamping at 20 ms **flickers**; re-stamping the **visible** buffer every **4 ms** drops the
> gap to ~12% → a stable overlay. Only stamp the visible buffer (the hidden one is rewritten by
> the game before the flip). The HID shmem keeps updating even while the game is paused, which
> is why you can read **touch** with the menu open.

---

## 6 · Saving configuration

`fs:USER` is in every game's ACL. Write a small file next to the `.3gx`: `fsInit` +
`FSUSER_OpenArchive(ARCHIVE_SDMC)` + `FSFILE_Write`, at
`sdmc:/luma/plugins/<TitleID>/Settings.cfg`. Persist favorites / toggles / hotkey / **theme** —
and **never** the cheat state. Use a `magic` + `version` in the blob. Prefer to **migrate** rather
than discard on a version bump: read whatever fields a valid `magic` gives you and default the rest,
so users keep their settings across updates (dropping the blob entirely resets everyone). Bigger,
label-keyed state (favorites, a completion tracker) is more robust in its **own text file** keyed by
a stable string, so adding or reordering items never invalidates saved entries.

> **How this template does it** — one constant, `PLUGIN_DIR`, and every path is built from it
> with `PlgPath("Settings.cfg")`. Set it to `"/luma/plugins/<your 16-hex Title ID>/"`; leave it
> empty and everything lands in `/luma/plugins/`, which works but is shared by every game.
>
> Note a *literal* `<YOUR_TITLE_ID>` placeholder cannot be used as the path: `<` and `>` are not
> legal FAT filename characters, so the file would simply fail to open.
>
> A previous version tried to discover the folder at runtime from `PluginHeader.pluginPathPA`.
> **It does not work**, and the failure is silent: that field is a *physical* address, and the
> `PA_PTR` mirror it needs is only valid for the IO region a plugin gets mapped (the HID
> register, say) — not for arbitrary FCRAM. On hardware the read never yielded a usable path, so
> it always fell through to the fallback and quietly wrote config to the shared folder. If you
> are tempted to make this automatic, verify it on a console before trusting it.

---

## 7 · The reusable engine modules

Everything below is **game-independent** — copy and reuse.

**Menu model.** `Item { label, cheat, folder, picker, desc, tool }` + `Folder { title, items,
count }`. Macros: `IT_CHEAT / IT_FOLDER / IT_PICKER / IT_TOOL / IT_SEP` (non-selectable section
header, `cheat == -2`). This is a **layout-agnostic** data model — `DrawMenuItem(it, x, y, cellW,
sel)` draws one row/cell wherever you place it. **How you lay out HOME (or any folder) is up to
you**: a plain scrolling list is the simplest starting point; a 2-column grid (`BuildRootLayout` +
`RootNeighbor`: left/right = column, up/down = row, skipping separators) is one richer option the
reference build uses, not a requirement. Themed category icons (13px) are optional decoration.

**Themes (live switch mechanism).** The trick: the color macros are **indirection to runtime
arrays**, so switching a theme never touches the hundreds of call sites:
```c
static u8 CGOLD[3] = {...}, CINK[3] = {...}, CGREEN[3] = {...}, CBG[3] = {...};
#define GOLD  CGOLD[0],CGOLD[1],CGOLD[2]         // same for INK, GREEN_ON, BG
void ApplyTheme(int i){ /* copy THEMES[i] -> CGOLD/CINK/... + optional backdrop-image flag */ }
```
`THEMES[]` is just a table of `{name, gold, ink, dim, green, bg}` structs — the **template ships
with exactly one entry**: a neutral monochrome starter (near-black background, white/light-grey
text, no colored accent) so a new plugin isn't born looking like Zelda. Add more themes only if
the plugin wants a theme picker at all; a single fixed palette is equally valid — the mechanism is
what's reusable, not the specific colors or count. A `parchment`-style flag is available if a
theme wants a background image instead of a flat fill, but nothing requires using it.

**Auto-contrast.** Text baked to a fixed colour vanishes on a theme whose background is the opposite
brightness. Guard against it by choosing ink from the *current* background's luminance: a
`ThemeBgLight()` check (bg luma above a threshold) picks dark text on light themes and light text on
dark ones. Apply it anywhere text sits on the theme background or a status pill, so **every** theme —
including ones added later — stays readable without per-theme tweaks.

**Tools** (they run with the game paused, so memory is stable):
- **Cheat Search** — known/unknown value, scan types (equal/greater/less/changed/inc/dec),
  memory-region filter, one-step **Undo**, poke. Unknown = a raw snapshot of the region compared
  on the next scan. Memory walk: `svcQueryMemory`, keep RW regions (state ≠ FREE/IO/RESERVED).
- **RAM Dumper** — dump a block to a `.bin` on the SD card. `ReadableSpan()` validates readable
  bytes so it never faults on an unmapped page.
- **Hex Editor** — a live hex grid with byte editing. `MemReadable`/`MemWritable` (region cache)
  protect read-only memory and show `--` for unmapped bytes.
- **On-screen keypad** (for entering an address/value on the touch screen) — each key is a
  rectangle you hit-test against the touch point, so the **art is independent of the logic**: draw
  keys as flat themed rects *or* skin them with sprites (section 4), with DEC/HEX toggle for hex
  entry. Change the look without touching the input handling.
- **Completion tracker (optional)** — a general per-item state tool: each item is `untouched /
  auto / manually-checked / manually-cleared`. **Auto-fill = sync to memory** (read each item's
  detection live and both *set* auto-marks it finds and *clear* stale auto-marks it no longer finds,
  while leaving the user's manual marks alone) so it always reflects the current game state, not a
  running tally. Persist it label-keyed (section 6). Handy for any "collectibles / progress" plugin;
  drop it if the game has nothing to track.

**Guide reader (optional)** — the same word-wrap + scroll + resume reader (remembers
category/page/scroll) works for either a **Plugin Guide** (generic, game-agnostic — how to use
*this* plugin's menu/tools) or a **Game Guide** (game-specific content, entirely optional and
only worth adding if the plugin wants one). Text can be embedded as C strings (a Python generator
turns a folder of `.txt` files into a header) or, for long content, loaded from the SD card at
runtime so it's editable without recompiling — same reader, either source.

**Localization (optional).** Same indirection idea as themes, applied to text instead of color:
every UI string is written once, in English, and wrapped in `T("English source")`. `T()` looks the
literal English string up in a small runtime table and returns a translation, or the English text
unchanged if there's no entry — so a partial translation degrades gracefully instead of showing
blanks or crashing.
```c
const char *T(const char *en) {           // table is empty until a language file loads
    for (int i = 0; i < g_langCount; ++i)
        if (strcmp(g_enKey[i], en) == 0) return g_trVal[i];
    return en;                             // no entry -> English fallback
}
```
The table is filled by parsing a plain-text file from the SD card,
`luma/plugins/<TitleID>/lang/<Name>.txt`, one `EnglishSource=Translation` mapping per line
(`#`/`;` = comment). English itself needs no file — it *is* the key, always available. A "Language"
setting (persisted like everything else in section 6) picks which file to load; reloading is just
re-parsing and re-populating the table, so switching language is instant and needs no restart.
Nothing about this requires more than one language — a plugin that never calls `T()` with a loaded
table just always shows English, at zero cost.

If the plugin uses this: **budget characters per context** before writing copy, and clip
defensively at the draw call (`CTextClip(..., maxWidthPx, ...)` truncates with `..` instead of
overflowing the window) — translations are reliably longer than English and there is no compiler
to catch a string that no longer fits its box. The same `T()` + SD-file pattern extends to the
guide reader (section 7): load a translated `game.txt`/`plugin.txt` per language, falling back to
the embedded English pages when no translation exists for the current language.

**Quick menu** — favorites (mark with Y in the menu), opened by a combo (e.g. L+SELECT). A favorite
can be a **cheat** (toggles in place) *or* a **shortcut** to a folder or a tool (opens it), so the
quick menu doubles as a launcher; each favorite type is one line, persisted label-keyed (section 6).
**`SELECT` = return to the game** from any screen (a global flag the sub-loops set); reopening
resumes where you left off, including re-entering the tool that was open.

**Rebindable in-game hotkeys.** Any hold-to-act cheat (fast-move, moon-jump…) can read its trigger
button from a small `hotKeys[]` table instead of a hard-coded constant; a Settings row cycles the
binding (A/X/Y/L/R) and a "reset hotkeys" entry restores defaults. The binding persists (section 6)
and the live glyph shows in the cheat's info via an inline button token (section 4).

---

## 8 · New-game checklist

1. **Title ID and region** (a 3DS Title ID database; confirm *which* region is yours — adjacent
   region numbers are easy to mix up). Install under `luma/plugins/<TitleID>/`.
2. **Cheat table** (section 3): an existing source or Cheat Search. **Region-specific.**
3. **Player base pointer** for base+offset cheats.
4. **Reuse the whole engine** (section 7) — none of it depends on the game.
5. **Change only:** the cheat table, sprites/art, and the Title ID in the config path. Everything
   else — menu layout, theme(s), whether to include a Game Guide, which (if any) languages beyond
   English, how many tools to expose — is a choice, not a requirement; add only what the plugin
   needs.
6. **Iterate on hardware.** Put the version on screen and bump it every build.

---

## 9 · Build & deploy (reference)

```sh
# build through the devkitPro msys2 shell (space-free path):
/c/devkitPro/msys2/usr/bin/bash.exe -lc 'cd <project> && make'
# deploy: copy the .3gx to the SD card, one per folder:
cp <project>/plugin.3gx  <SD>/luma/plugins/<TitleID>/<Name>.3gx
```

---

> **What you get for free** — about 90% is game-independent. A new plugin is: **cheats + art +
> Title ID**. The hard part — overlay rendering, the system font, pausing, persistence, the menu,
> themes and tools in a self-rendered plugin — is solved and documented here, and doesn't change
> from game to game.
>
> Technique credits: rendering approach informed by CTRPluginFramework; loader by Luma3DS /
> PabloMK7. 3DS hardware/kernel addresses hold for any game; the `<like-this>` values you
> re-anchor per game and version.

---

## Make it yours — a checklist

Work down this list and you have your own plugin. Everything not listed is engine you inherit
as-is.

**1. Name it.**
- `Makefile` → `TARGET` (the output `.3gx` filename) and `PLGINFO`.
- Rename `CTRComposer.plgInfo` to match `PLGINFO`; set `Author`, `Title`, `Summary`.
- `Sources/main.c` → `PLUGIN_NAME`, `PLUGIN_VER`, `PLUGIN_TAG`.

**2. Find your Title ID** (section 8). Install under `luma/plugins/<TitleID>/`. Optionally add
it to `Targets:` in the `.plgInfo` so the plugin only loads for that game. You do *not* need to
put it in the source — the plugin discovers its own folder at runtime.

**3. Write your cheat table** — the one genuinely game-specific job (section 3).
- Get addresses from an existing source (a `.plg`, an AR code bank, a community plugin) or find
  them with the built-in **Cheat Search**. They are **region- and version-specific**.
- In `Sources/main.c`: add `CH_*` entries to the cheat enum, rows to a `Folder`, and the writes
  to `ApplyCheats()` (continuous) or `OneShot()` (applied once).
- Set `EXAMPLE_ENABLED` to `1` only after you have replaced the placeholder addresses, and
  delete the `CH_EX_*` examples once you no longer need them.
- Update `IsToggleCheat()` so on/off cheats get a checkbox and one-shots get a plain box.

**4. Lay out your menu.** `folders[]` and the `Item` rows are pure data. HOME is a 2-column grid
and everything else is a list — that is a choice made in one line of `ComposeMenu()`, not a rule.

**5. Theme it.** Add rows to `THEMES[]` in `Includes/themes.h`, or ship the single neutral one.
Auto-contrast keeps text readable on light and dark backgrounds without per-theme tweaking.

**6. Art (optional).** The template is art-free: vector icons drawn from primitives, a
code-drawn keypad, no logo. To add real art, embed it as RGBA4444 and blit with `DrawImg()` or
`DrawScaled()` — and **pack with round-to-nearest**, `clamp((c + 8) / 17, 0, 15)`, or your art
comes out visibly brighter than the source. `Assets/gen_glyphs.py` is a worked example.

**7. Guides and languages (optional).** Replace the placeholder pages in `Includes/guide.h`, or
drop `guide/English/game.txt` on the SD card and skip C entirely. Translations are
`lang/<Name>.txt` files keyed by the English string; missing entries fall back to English.

**8. Tracker (optional).** Fill `CHK_CATS` with your collectibles and give each a detection kind
(`CK_BIT` / `CK_BYTEEQ` / `CK_NONZERO`), or leave it as-is, or delete the row from `rootItems[]`.

**9. Iterate on hardware.** Bump `PLUGIN_VER` **every** build and check it on screen — it is
your proof that the `.3gx` on the SD card is the one you just compiled. Verify the bump actually
landed; a blind `sed` can silently no-op and freeze the on-screen version.

### Build gotchas worth re-reading

- Build from a **space-free path**, through the **devkitPro msys2 shell** — not git-bash.
  A space gives you `make[1]: /d/My Project: Is a directory`.
- **One `.3gx` per plugin folder.**
- **3gxtool 1.3** writes the `3GX$0002` container Luma expects; `3GX$0001` is rejected.
- Never call `hidInit()` — on New 3DS it pulls in `irrst` and freezes the game.
- Assume **ZL/ZR are unreachable** from a plugin. Use L/R combos.

### When something misbehaves, prove the plugin is involved first

Before debugging your plugin, **reproduce the fault with the plugin deleted from the SD card.**

This is not hypothetical advice. A hang on relaunching one particular game was chased through
the engine for hours here — exit handshakes, memory reservations, leaked handles, binary size —
before the obvious test was run. With the `.3gx` removed entirely, the game hung exactly the
same way: it was the game and the screen-streaming CFW in use, and the plugin had never been
part of it. One 30-second test would have replaced all of that.

Running inside someone else's process makes a plugin a magnet for blame. Make it prove its
guilt.

## Continuous integration

`.github/workflows/build.yml` builds **both** `.3gx` files from the same tree on every push and
PR, and uploads them as run artifacts you can download from the Actions tab. On a `v*` tag it
also creates a GitHub Release with both attached.

It runs in the official `devkitpro/devkitarm` container and builds `3gxtool` from source, since
that tool is not part of the image. It asserts each output really is a `3GX$0002` container —
the older `3GX$0001` builds fine and then silently refuses to load — and it fails the build if
`default.3gx` ever mentions a menu it doesn't have.

```sh
git tag v1.0.0 && git push origin v1.0.0   # cut a release
```

## Credits

CTRComposer owes a great deal to **CTRPluginFramework** and the earlier community `.plg`
plugins — those projects made 3DS plugins possible and are the direct inspiration here.

- Plugin loader: **Luma3DS** (LumaTeam)
- `.3gx` format and `3gxtool`: **PabloMK7** / **Nanquitas**
- Repository structure modelled on **PabloMK7/CTRPluginFramework-BlankTemplate**
- Small 6×10 bitmap font: the **Linux console font** (`lib/fonts/font_6x10.c`)
- Button glyphs: original, generated by `Assets/gen_glyphs.py`

If you build on someone's address map, save-data research, art or translation, credit them —
there is an About screen and a guide Credits page waiting for exactly that.

## License

[MIT](LICENSE). Plugins you build from this template are yours; the license only covers the
template itself. Game names and game content belong to their publishers.
