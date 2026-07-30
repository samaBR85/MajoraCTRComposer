#ifndef GUIDE_H
#define GUIDE_H

// Embedded content for the Game Guide reader.
//
// The reader itself (word-wrap, scroll, resume-where-you-left-off) is engine code in
// main.c and is completely game-agnostic - it just renders whatever pages it is handed.
// This header is the EMBEDDED English fallback. Two ways to fill it with real content:
//
//   1. Edit this file. Each page body is a plain C string; '\n' starts a new line and the
//      reader word-wraps the rest. A small Python script that turns a folder of .txt files
//      into this header is the usual way to manage anything long.
//
//   2. Don't touch C at all: drop a text file at
//         sdmc:/luma/plugins/<TitleID>/guide/English/game.txt
//      and it replaces these pages at runtime. Format:
//         %C Category Name
//         %P Page Name
//         ...body lines...
//      That is also how translations work - guide/Francais/game.txt, etc. Loading from SD
//      keeps the .3gx small and lets you edit the guide without recompiling.
//
// The pages below are deliberately generic placeholders: no game content ships with the
// template. Delete them and write your own, or delete the Game Guide row from rootItems[]
// in main.c if your plugin has nothing to document.

typedef struct { const char *title; const char *body; } GuidePage;
typedef struct { const char *title; const GuidePage *pages; int nPages; } GuideCat;

static const GuidePage GUIDE_C0[] = {
    { "This guide is empty",
      "Nothing here is game content - these are placeholder pages that ship with the\n"
      "CTRComposer blank template.\n"
      "\n"
      "You are looking at the guide reader: a scrollable, word-wrapped text viewer with\n"
      "categories, pages, and resume. It remembers which category, page and scroll\n"
      "position you were on, so leaving and reopening puts you back where you were.\n"
      "\n"
      "Replace this content with your own - see the comments at the top of\n"
      "Includes/guide.h for the two ways to do it." },
    { "Adding pages in C",
      "Each category is an array of GuidePage, and each page is just a title and a body\n"
      "string. Add a page by adding an entry to one of the GUIDE_C* arrays below, then\n"
      "make sure the category's entry in GUIDE_CATS[] still counts correctly - it uses\n"
      "sizeof, so it updates itself.\n"
      "\n"
      "Bodies are plain text. A '\\n' forces a line break; everything else wraps to the\n"
      "window width automatically, so you do not need to hand-wrap your paragraphs." },
};

static const GuidePage GUIDE_C1[] = {
    { "Loading a guide from the SD card",
      "For anything long, keep the text off the binary. Create:\n"
      "\n"
      "  luma/plugins/<TitleID>/guide/English/game.txt\n"
      "\n"
      "and write it like this:\n"
      "\n"
      "  %C Getting Started\n"
      "  %P First Steps\n"
      "  Body text goes here, as many lines as you want.\n"
      "  %P Second Page\n"
      "  More body text.\n"
      "  %C Another Category\n"
      "  %P A Page In It\n"
      "  And so on.\n"
      "\n"
      "If that file parses, it replaces these embedded pages entirely. If it is missing or\n"
      "malformed, the embedded pages stay - so a broken SD file can never leave the reader\n"
      "blank." },
    { "Translating the guide",
      "Swap 'English' in the path for any name listed in kLangNames[] in main.c:\n"
      "\n"
      "  luma/plugins/<TitleID>/guide/Francais/game.txt\n"
      "\n"
      "The guide follows the Language setting. When no file exists for the selected\n"
      "language, the reader falls back to the embedded English pages rather than showing\n"
      "nothing - the same graceful-degradation rule the T() string table uses." },
};

static const GuideCat GUIDE_CATS[] = {
    { "About This Reader", GUIDE_C0, (int)(sizeof(GUIDE_C0)/sizeof(GUIDE_C0[0])) },
    { "Filling It In",     GUIDE_C1, (int)(sizeof(GUIDE_C1)/sizeof(GUIDE_C1[0])) },
};
#define GUIDE_NCATS ((int)(sizeof(GUIDE_CATS)/sizeof(GUIDE_CATS[0])))

#endif
