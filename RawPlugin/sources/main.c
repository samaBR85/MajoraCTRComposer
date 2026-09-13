#include <3ds.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "plgldr.h"
#include "csvc.h"
#include "common.h"
#include "font6x10.h"
#include "sysfont.h"
#include "glyphs.h"
#include "guide.h"
#include "themes.h"
#include "sprites.h"
#include "topbg.h"
#include "botbg.h"
#include "logo.h"

#include "plugin/identity.inc.c"

#include "engine/platform.inc.c"

#include "engine/render.inc.c"

#include "plugin/cheat_ids.inc.c"

#include "engine/storage.inc.c"

#include "plugin/cheats.inc.c"

#include "plugin/pickers.inc.c"

#include "engine/menu_model.inc.c"

#include "plugin/menu_tables.inc.c"

#include "engine/menu_nav.inc.c"

#include "engine/favorites.inc.c"


#include "engine/theme.inc.c"

#include "engine/sprites.inc.c"

#include "plugin/cheat_icons.inc.c"

#include "engine/icons.inc.c"

#include "engine/bottom_screen.inc.c"

#include "engine/toast.inc.c"

#include "engine/menu_render.inc.c"

#include "engine/tools.inc.c"
#include "engine/guide_reader.inc.c"

#if !TOOLS_ONLY
#include "plugin/guide_text.inc.c"
#include "engine/guide_game.inc.c"
#endif

#include "engine/guide_plugin.inc.c"

#if !TOOLS_ONLY
#include "engine/tracker.inc.c"
#include "plugin/tracker_data.inc.c"
#include "engine/tracker_ui.inc.c"
#endif // !TOOLS_ONLY

#include "engine/tool_dispatch.inc.c"

#include "engine/menu_loop.inc.c"
#include "engine/quick_menu.inc.c"
// ===================== Thread / entry =====================

// Release everything we hold, in the reverse order we took it. Called when Luma tells us the
// game is exiting, BEFORE we signal that the loader may continue.
__attribute__((unused)) static void PluginShutdown(void)
{
    // If we are torn down with the menu open the game's threads are still suspended. Never let
    // a process try to exit with its own threads frozen.
    ResumeGame();

    // PROOF that this path ran. There is no screen to draw on at exit, so drop a marker file
    // next to the .3gx instead. If it appears after closing the game, Luma really did signal
    // onProcessExitEvent and the handshake is live; if it never appears, the event was never
    // delivered and the whole block is dead weight. Cheap, and it settles the question.
    if (fsReady)
    {
        Handle f;
        if (R_SUCCEEDED(FSUSER_OpenFile(&f, cfgArchive,
                fsMakePath(PATH_ASCII, PlgPath("exit_handshake_ran.txt")),
                FS_OPEN_WRITE | FS_OPEN_CREATE, 0)))
        {
            static const char msg[] = "PluginShutdown() ran; resumeExitEvent was signalled.\n";
            u32 wrote = 0;
            FSFILE_SetSize(f, sizeof(msg) - 1);
            FSFILE_Write(f, &wrote, 0, msg, sizeof(msg) - 1, FS_WRITE_FLUSH);
            FSFILE_Close(f);
        }
    }

    // Last chance to persist anything the user changed and we had not written yet.
    if (configDirty) { ConfigSave(); configDirty = 0; }
    if (favDirty)    { FavSave();    favDirty = 0; }

    if (hidShmem && g_hidMem)          // touch-panel shared memory (manual hid:USER init)
    {
        svcUnmapMemoryBlock(g_hidMem, (u32)hidShmem);
        svcCloseHandle(g_hidMem);
        g_hidMem = 0; hidShmem = NULL; hidReady = 0;
    }
    if (fsReady)                            // SD archive + fs session
    {
        FSUSER_CloseArchive(cfgArchive);
        fsExit();
        fsReady = 0;
    }
    plgLdrExit();
}

void ThreadMain(void *arg)
{
    (void)arg;        // the loader's thread signature passes one; this plugin has no use for it
    InitThreadVars(); // must run before any newlib/hid/fs call on this thread

    // Make the whole game process RWX up front, exactly like CTRPluginFramework does at init.
    // Many ported cheats write to the game's read-only segments (const tables in .rodata, code in
    // .text). Under CTRPF those writes worked *only* because CTRPF had already flipped the process
    // to RWX globally; a raw plugin defaults to RO there, so those writes silently no-op. Doing the
    // same flip once here restores the CTRPF behavior for every cheat at once (Can Use All Items,
    // and any other .rodata/.text write), instead of patching RWX in one cheat at a time. This can
    // only ENABLE previously-failing writes to RO pages - writes to already-writable RAM are
    // unaffected - so it never breaks a cheat that already worked.
    svcControlProcess(CUR_PROCESS_HANDLE, PROCESSOP_SET_MMU_TO_RWX, 0, 0);

    gCompose = (u8 *)malloc(TOP_W * TOP_H * 3);
    savedBot = (u16 *)malloc(BOT_W * BOT_H * 2);
    savedTop = (u16 *)malloc(TOP_W * TOP_H * 2);
    ApplyTheme(1); // seed the live colors from THEMES[1] (Termina, the shipped default) BEFORE
                   // anything can draw. A fresh install has no Settings.cfg, so ConfigLoad never
                   // calls ApplyTheme; without this the menu would run on whatever the CINK/CBG
                   // initializers happen to hold. Users who pick a theme keep it (saved to config).
    ConfigLoad(); // restore toast toggle + quick-menu hotkey + theme + language from SD
    FavLoad();    // restore favorites (own label-keyed file, survives cheat-list changes)

    u32 prev = HID_PAD;
    int comboPrev = 0;
    while (1)
    {
        // 4ms while a toast is on screen (fast re-stamp), 20ms otherwise
        svcSleepThread((toastTicks > 0 ? 4 : 20) * 1000 * 1000);

#if EXIT_HANDSHAKE
        // Luma signals onProcessExitEvent when the game is shutting down. The 3gx contract
        // appears to be: clean up, then signal resumeExitEvent so the loader can finish tearing
        // the plugin down.
        //
        // DISABLED BY DEFAULT because it was TESTED AND DOES NOT RUN. Built with this on, and
        // with PluginShutdown() writing a marker file to the SD card as proof, the marker never
        // appeared after closing the game - while Settings.cfg written by the same code path
        // did. So the file I/O was fine and this block simply never executed:
        // PROCESSOP_GET_ON_EXIT_EVENT in main() does not hand back a usable event, and
        // onProcessExitEvent stays 0.
        //
        // The reference plugin this engine came from ignores these events too, and shows no
        // teardown problems, so nothing is lost. Kept behind the flag in case a future Luma
        // build starts delivering the event - the marker file makes that a one-run check.
        if (onProcessExitEvent && svcWaitSynchronization(onProcessExitEvent, 0) == 0)
        {
            PluginShutdown();
            if (resumeExitEvent) svcSignalEvent(resumeExitEvent);
            svcExitThread();   // does not return
        }
#endif
        u32 pad = HID_PAD, down = ARepeat(pad, &prev, &g_arHold);

        const QmCombo *qc = &qmCombos[qmCombo];
        int comboNow = (pad & qc->pad) == qc->pad;

        if (comboNow && !comboPrev)
        {
            QuickMenu();
            prev = HID_PAD;
            comboNow = 1; // treat as still held: no instant reopen
            if (g_openFolder >= 0) // a folder shortcut was picked in the quick menu -> open it
            {
                int fld = g_openFolder; g_openFolder = -1;
                // Save the normal menu position and restore it afterwards, so this transient jump
                // doesn't hijack where SELECT reopens (else SELECT would keep landing in this folder).
                int sD = menuDepth, sF = menuFolder, sC = menuCursor, sS = menuScroll;
                menuDepth = 0; menuFolder = fld; menuScroll = 0; menuCursor = 0;
                { const Folder *nf = &folders[fld]; // land on the first selectable row
                  while (menuCursor < nf->count && IS_SEP(&nf->items[menuCursor])) menuCursor++;
                  if (menuCursor >= nf->count) menuCursor = 0; }
                g_qmHandoff = 1;   // the quick menu is still on screen: do not recapture it as backdrop
                RunMenu();
                menuDepth = sD; menuFolder = sF; menuCursor = sC; menuScroll = sS;
                prev = HID_PAD;
            }
            else if (g_openTool >= 0) // a tool shortcut was picked in the quick menu -> launch it
            {
                int t = g_openTool; g_openTool = -1;
                int sD = menuDepth, sF = menuFolder, sC = menuCursor, sS = menuScroll;
                g_resumeTool = t;                    // RunMenu's resume path runs the tool with full setup
                menuDepth = 0; menuFolder = 0; menuCursor = 0; menuScroll = 0; // land on HOME after the tool
                { const Folder *nf = &folders[F_ROOT]; // first selectable row, not a separator
                  while (menuCursor < nf->count && IS_SEP(&nf->items[menuCursor])) menuCursor++;
                  if (menuCursor >= nf->count) menuCursor = 0; }
                g_qmHandoff = 1;   // the quick menu is still on screen: do not recapture it as backdrop
                RunMenu();
                menuDepth = sD; menuFolder = sF; menuCursor = sC; menuScroll = sS;
                prev = HID_PAD;
            }
        }
        else if ((down & BUTTON_SELECT) && !comboNow)
        {
            RunMenu();
            prev = HID_PAD;
        }
        comboPrev = comboNow;

        ApplyCheats();
        ToastTick();
    }
}

// Normally provided by the 3dsx crt0; NULL = "no homebrew env, use real srv"
void *__service_ptr = NULL;

extern char* fake_heap_start;
extern char* fake_heap_end;
extern u32 __ctru_heap;
extern u32 __ctru_linear_heap;
u32 __ctru_heap_size = 0;
u32 __ctru_linear_heap_size = 0;

void __system_allocateHeaps(PluginHeader *header)
{
    __ctru_heap_size = header->heapSize;
    __ctru_heap = header->heapVA;
    fake_heap_start = (char *)__ctru_heap;
    fake_heap_end = fake_heap_start + __ctru_heap_size;
}

void main(void)
{
    PluginHeader *header = (PluginHeader *)0x07000000;
    if (header->magic != HeaderMagic) return;
    __system_allocateHeaps(header);
    cheatState[CH_CFG_TOAST] = 1;    // notifications on by default
    cheatState[CH_CFG_AUTOFILL] = 1; // auto-fill the Checklist on open by default
    srvInit();
    plgLdrInit();
    svcControlProcess(CUR_PROCESS_HANDLE, PROCESSOP_GET_ON_EXIT_EVENT, (u32)&onProcessExitEvent, (u32)&resumeExitEvent);
    svcCreateThread(&thread, ThreadMain, 0, (u32 *)(stack + PLG_STACK_SIZE), 30, -1);
}
