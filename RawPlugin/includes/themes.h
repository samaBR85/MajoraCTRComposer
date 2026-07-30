#ifndef THEMES_H
#define THEMES_H

// CTRComposer theme table.
//
// The engine's color macros (GOLD / INK / INK_DIM / GREEN_ON / BG in main.c) expand to
// RUNTIME arrays, not constants. ApplyTheme() copies one row of this table into those
// arrays, so switching a theme instantly recolors every menu without touching any of the
// hundreds of draw call sites. That indirection is the reusable part - the specific
// colors and how many you ship are entirely your choice.
//
// Roles:
//   gold  - titles, accents, the selection highlight
//   ink   - primary text
//   dim   - secondary / unselected text
//   green - "enabled" state (checkboxes, ON pills)
//   bg    - window background
//   parchment - 1 = draw a background IMAGE instead of a flat fill. The template ships no
//               background art, so leave this 0 unless you add your own and wire it up in
//               ComposeBackdrop().
//
// The template ships ONE neutral monochrome theme on purpose, so a new plugin isn't born
// wearing someone else's palette. Add as many rows as you like - or none, and delete the
// theme picker entirely. Auto-contrast (ThemeBgLight() in main.c) keeps text readable on
// light and dark backgrounds alike, so new themes need no per-theme tweaking.

typedef struct { const char *name; u8 gold[3], ink[3], dim[3], green[3], bg[3]; u8 parchment; } Theme;

static const Theme THEMES[] = {
    //  name         gold             ink              dim              green            bg          parchment
    { "Neutral", {230,230,230}, {242,242,242}, {150,150,150}, {200,200,200}, {18,18,20}, 0 },
};
#define THEME_COUNT ((int)(sizeof(THEMES)/sizeof(THEMES[0])))

#endif
