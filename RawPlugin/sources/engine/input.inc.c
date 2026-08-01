// ---- D-pad auto-repeat (typematic) -------------------------------------------------------------
// Menu loops normally use edge detection (pad & ~prev), so a held button fires once. This wraps
// that: hold a D-pad direction and, after a short delay, it keeps firing so long lists scroll
// without mashing. ONLY the D-pad repeats - A/B/X/Y/START/SELECT stay edge-only (else a held A
// would toggle a cheat over and over). Each loop passes its own `prev`; `hold` is shared (only one
// loop runs at a time, and it resets whenever no direction is held).
#define AR_DIRS  (BUTTON_UP | BUTTON_DOWN | BUTTON_LEFT | BUTTON_RIGHT)
#define AR_DELAY 20   // frames a direction is held before auto-repeat kicks in (~320ms @ 16ms/frame)
#define AR_RATE  4    // then repeat every this many frames (~65ms)
static int g_arHold = 0;
static u32 ARepeat(u32 pad, u32 *prev, int *hold)
{
    u32 down = pad & ~*prev;                 // genuine edges (any button)
    u32 dir  = pad & AR_DIRS;
    if (dir && dir == (*prev & AR_DIRS))     // same direction(s) still held since last frame
    {
        if (++(*hold) >= AR_DELAY && ((*hold - AR_DELAY) % AR_RATE) == 0) down |= dir;
    }
    else *hold = 0;                          // direction changed or released -> restart the delay
    *prev = pad;
    return down;
}

// Wait for the buttons that closed a screen to be physically released. The game is paused while
// the plugin owns the screen; the instant it resumes it reads the live pad, so a still-held
// B/SELECT would fire in-game (B = sword swing). Capped (~2s) so a stuck pad can't hang the
// console. Lives here, next to ARepeat, because it is input plumbing - every screen needs it,
// including the info box and About, which come earlier in this file than the menu.
static void DrainButtons(u32 mask)
{
    for (int i = 0; i < 125 && (HID_PAD & mask); ++i)
        svcSleepThread(16 * 1000 * 1000);
}
