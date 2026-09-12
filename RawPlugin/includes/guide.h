#ifndef GUIDE_H
#define GUIDE_H

// Embedded English fallback for the Game Guide reader. Normally overridden at runtime
// by guide/<Language>/game.txt on the SD card (see GuideLoad): a valid game.txt replaces
// these pages entirely, a missing or malformed one leaves them - so the reader is never
// blank. Keep this fallback short; the full walkthrough lives on the SD card, off the binary.

typedef struct { const char *title; const char *body; } GuidePage;
typedef struct { const char *title; const GuidePage *pages; int nPages; } GuideCat;

static const GuidePage GUIDE_C0[] = {
    { "The Three-Day Cycle",
      "Termina runs on a 72-hour clock and the moon falls at the end of the third day.\n"
      "Resetting with the Song of Time wipes consumables and dungeon progress, but keeps\n"
      "the things that matter: masks, songs, the four boss remains, and your sword,\n"
      "quiver and bomb-bag upgrades.\n"
      "\n"
      "Play each cycle toward one goal - a temple or a long side quest - then bank the\n"
      "reward and reset. The Inverted Song of Time slows the clock; the Song of Double\n"
      "Time skips to the next dawn or dusk." },
    { "The Four Temples",
      "- Woodfall (Southern Swamp): Hero's Bow, boss Odolwa.\n"
      "- Snowhead (mountains): Fire Arrow, boss Goht.\n"
      "- Great Bay (coast, as Zora): Ice Arrow, boss Gyorg.\n"
      "- Stone Tower (Ikana Canyon): Light Arrows, boss Twinmold.\n"
      "\n"
      "The dungeon item you just found is almost always the answer to the room that just\n"
      "stopped you. Stone Tower flips upside down with Light Arrows - most of its dead\n"
      "ends are rooms asking to be inverted.\n"
      "\n"
      "For the fuller guide, drop guide/English/game.txt into the plugin's SD folder." },
};

static const GuideCat GUIDE_CATS[] = {
    { "Majora's Mask 3D", GUIDE_C0, (int)(sizeof(GUIDE_C0)/sizeof(GUIDE_C0[0])) },
};
#define GUIDE_NCATS ((int)(sizeof(GUIDE_CATS)/sizeof(GUIDE_CATS[0])))

#endif
