// ===================== Completion tracker (per-item progress) =====================
// A general "collectibles / progress" tool, and one of the more useful things the engine
// gives you for free. Each item has one of four states:
//     0 untouched   1 auto-detected   2 you checked it   3 you cleared it
//
// AUTO-FILL = SYNC TO MEMORY, not a running tally. When it runs it re-reads every
// detectable item and both SETS marks it finds and CLEARS stale auto-marks it no longer
// finds, while never touching a mark you made by hand. That is what keeps the tool honest
// after a save reload: it always reflects the CURRENT game state.
//
// Detection kinds - all read-only, and all keyed off addresses you supply:
//   CK_MANUAL  - no memory signal; only you can tick it
//   CK_BIT     - (R8(addr) & mask) != 0            a flag bit inside a byte
//   CK_BYTEEQ  - R8(addr) == mask                  an exact byte value at a FIXED position
//   CK_NONZERO - R8(addr) != 0                     "the slot is filled"
//   CK_SCANEQ  - any R8(addr..addr+scanLen-1) == mask   value appears SOMEWHERE in a range
// Add your own kind by extending this enum and ChecklistAutoFill() together.
//
// >>> THE TABLE BELOW IS EMPTY OF GAME DATA. <<<
// It ships with one "Example" category whose rows are placeholders pointing at address 0,
// purely so the tool is explorable before you have any addresses. Replace CHK_CATS with
// your game's collectibles - it is pure data, and the UI below neither knows nor cares how
// many categories or items there are. If your game has nothing to track, delete the Tracker
// row from rootItems[] and this whole section.
//
// Progress is persisted KEYED BY THE `key` STRING, not by position, so you can add, remove
// and reorder items freely without invalidating anyone's saved progress. Never reuse a key
// for a different item.
enum { CK_MANUAL = 0, CK_BIT = 1, CK_BYTEEQ = 2, CK_NONZERO = 3, CK_SCANEQ = 4 };
// CKI_SPRITE: iconArg is an MSPR_* key from sprites.h - a real rip, not hand-drawn.
// CKI_SWATCH: iconArg is a packed RGB565 color (((r>>3)<<11)|((g>>2)<<5)|(b>>3)) - a plain
// tinted square for items with no matching art on the sheet, in a color close to the name.
enum { CKI_HEART, CKI_SKULL, CKI_NOTE, CKI_KEYITEM, CKI_NONE, CKI_SPRITE, CKI_SWATCH };
#define CHKRGB(r,g,b) (u16)((((r)>>3)<<11)|(((g)>>2)<<5)|((b)>>3))

typedef struct {
    const char *key;    // stable save-key, never shown (so item text can be edited freely later)
    const char *task, *hint, *loc; // loc = "" -> no location to reveal
    u8 iconKind; u16 iconArg; u8 kind;
    u32 addr; u8 mask;
    u8 scanLen;     // CK_SCANEQ only: how many bytes from addr to scan (0/unused for every other kind)
    u16 iconArgBig; // CKI_SPRITE only, optional: a DIFFERENT sprite key for the detail card (cell
                     // > 3) than the list rows use - 0 means "same sprite both places" (the norm;
                     // Owl Statues are the only category that wants a distinct bigger render here)
} ChkItem;
typedef struct { const char *name; const ChkItem *items; int count; } ChkCat;
