// Masks (24): CONFIRMED address range 0x77636C-0x776383, 24 bytes total. IMPORTANT: these are
// filled in ACQUISITION ORDER, not one fixed slot per mask - the "Have all Masks" cheat and the
// 100% save cross-check both read 0x32-0x49 ascending only because that save happened to collect
// them in id order. A real save can have any mask's id in any of the 24 slots. So detection scans
// the whole 24-byte range for the target id rather than checking a single fixed offset (this bug
// is exactly why the first version of this Tracker read 0/24 against a real 100% save - fixed by
// switching to CK_SCANEQ). Item ids/names are from the zeldaret/mm N64 decompilation
// (include/z64item.h's ItemId enum), which also matches our own independently-confirmed B-Button
// value for Fierce Deity's Mask (0x35) - so this table is trusted over the earlier community AR
// list. Hints are kept to well-known facts only; anything we weren't confident enough to state
// precisely is left generic rather than risk a wrong walkthrough step.
#define MASKS_BASE 0x77636C
#define MASKS_LEN  24
static const ChkItem CK_MASKS[] = {
    // key                task                     hint                                                                          loc  icon         arg kind       addr        mask  scanLen
    { "mask_deku",        "Deku Mask",              "One of the four transformation masks.",                                       "", CKI_SPRITE, MSPR_MASK_DEKU, CK_SCANEQ, MASKS_BASE, 0x32, MASKS_LEN },
    { "mask_goron",       "Goron Mask",             "One of the four transformation masks.",                                       "", CKI_SPRITE, MSPR_MASK_GORON, CK_SCANEQ, MASKS_BASE, 0x33, MASKS_LEN },
    { "mask_zora",        "Zora Mask",              "One of the four transformation masks.",                                       "", CKI_SPRITE, MSPR_ZORA_MASK, CK_SCANEQ, MASKS_BASE, 0x34, MASKS_LEN },
    { "mask_fierce",      "Fierce Deity's Mask",    "Requires every other mask first.",                                            "", CKI_SPRITE, MSPR_FD_MASK, CK_SCANEQ, MASKS_BASE, 0x35, MASKS_LEN },
    { "mask_truth",       "Mask of Truth",          "Reward for completing every Bombers' Notebook entry.",                        "", CKI_SPRITE, MSPR_MASK_TRUTH, CK_SCANEQ, MASKS_BASE, 0x36, MASKS_LEN },
    { "mask_kafei",       "Kafei's Mask",           "Part of the Anju & Kafei quest.",                                             "", CKI_SPRITE, MSPR_MASK_KAFEI, CK_SCANEQ, MASKS_BASE, 0x37, MASKS_LEN },
    { "mask_allnight",    "All-Night Mask",         "Part of the Romani Ranch quest line.",                                        "", CKI_SPRITE, MSPR_MASK_ALLNIGHT, CK_SCANEQ, MASKS_BASE, 0x38, MASKS_LEN },
    { "mask_bunny",       "Bunny Hood",             "Won from a side quest reward.",                                               "", CKI_SPRITE, MSPR_MASK_BUNNY, CK_SCANEQ, MASKS_BASE, 0x39, MASKS_LEN },
    { "mask_keaton",      "Keaton Mask",            "Reward for correctly answering Keaton's riddles.",                            "", CKI_SPRITE, MSPR_MASK_KEATON, CK_SCANEQ, MASKS_BASE, 0x3A, MASKS_LEN },
    { "mask_garo",        "Garo's Mask",            "Reward from a Garo Master in Ikana Canyon.",                                  "", CKI_SPRITE, MSPR_GARO_MASK, CK_SCANEQ, MASKS_BASE, 0x3B, MASKS_LEN },
    { "mask_romani",      "Romani's Mask",          "Part of the Romani Ranch quest line.",                                        "", CKI_SPRITE, MSPR_MASK_ROMANI, CK_SCANEQ, MASKS_BASE, 0x3C, MASKS_LEN },
    { "mask_circus",      "Troupe Leader's Mask",   "Reward from Gorman for the milk delivery side quest.", "", CKI_SPRITE, MSPR_MASK_TROUPE, CK_SCANEQ, MASKS_BASE, 0x3D, MASKS_LEN },
    { "mask_postman",     "Postman's Hat",          "Reward for helping the Postman.",                                             "", CKI_SPRITE, MSPR_MASK_POSTMAN, CK_SCANEQ, MASKS_BASE, 0x3E, MASKS_LEN },
    { "mask_couple",      "Couple's Mask",          "Final reward of the Anju & Kafei quest.",                                     "", CKI_SPRITE, MSPR_MASK_COUPLE, CK_SCANEQ, MASKS_BASE, 0x3F, MASKS_LEN },
    { "mask_greatfairy",  "Great Fairy's Mask",     "Reward from the Clock Town Great Fairy for collecting Stray Fairies.",        "", CKI_SPRITE, MSPR_MASK_GREATFAIRY, CK_SCANEQ, MASKS_BASE, 0x40, MASKS_LEN },
    { "mask_gibdo",       "Gibdo Mask",             "Given by the Gibdos in the Ikana royal crypt.",                               "", CKI_SPRITE, MSPR_MASK_GIBDO, CK_SCANEQ, MASKS_BASE, 0x41, MASKS_LEN },
    { "mask_dongero",     "Don Gero's Mask",        "Reward for gathering the frogs at the Woodfall spring.",                      "", CKI_SPRITE, MSPR_MASK_DONGERO, CK_SCANEQ, MASKS_BASE, 0x42, MASKS_LEN },
    { "mask_kamaro",      "Kamaro's Mask",          "Left behind after learning Kamaro's dance.",                                  "", CKI_SPRITE, MSPR_MASK_KAMARO, CK_SCANEQ, MASKS_BASE, 0x43, MASKS_LEN },
    { "mask_captain",     "Captain's Hat",          "Found inside the Pirates' Fortress.",                                         "", CKI_SPRITE, MSPR_MASK_CAPTAIN, CK_SCANEQ, MASKS_BASE, 0x44, MASKS_LEN },
    { "mask_stone",       "Stone Mask",             "Makes most enemies and NPCs ignore you.",                                     "", CKI_SPRITE, MSPR_MASK_STONE, CK_SCANEQ, MASKS_BASE, 0x45, MASKS_LEN },
    { "mask_bremen",      "Bremen Mask",            "Makes young animals march behind you.",                                       "", CKI_SPRITE, MSPR_MASK_BREMEN, CK_SCANEQ, MASKS_BASE, 0x46, MASKS_LEN },
    { "mask_blast",       "Blast Mask",             "Reward from the West Clock Town bomb shop owner.",                            "", CKI_SPRITE, MSPR_MASK_BLAST, CK_SCANEQ, MASKS_BASE, 0x47, MASKS_LEN },
    { "mask_scents",      "Mask of Scents",         "Lets you smell nearby hidden things.",                                        "", CKI_SPRITE, MSPR_MASK_SCENTS, CK_SCANEQ, MASKS_BASE, 0x48, MASKS_LEN },
    { "mask_giant",       "Giant's Mask",           "Turns Link giant-sized - required for the final boss.",                       "", CKI_SPRITE, MSPR_MASK_GIANT, CK_SCANEQ, MASKS_BASE, 0x49, MASKS_LEN },
};

// Stray Fairies (4 dungeons): CONFIRMED address range 0x7763E8-0x7763EB, one byte per dungeon
// holding a 0-15 count (not a bitmask - "All Stray Fairies" fills each with 0x0F = 15). Tracking
// is per-dungeon (15/15), not per-fairy - the save data doesn't expose which specific fairy.
// Auto-fill CONFIRMED on hardware: applied "All Stray Fairies", saved, reloaded - the Checklist
// read all 4 dungeon entries as done (4/5, only the manual Clock Town fairy left unchecked).
static const ChkItem CK_FAIRIES[] = {
    // key             task                          hint                                                                    loc icon      arg kind        addr      mask
    { "fairy_woodfall", "Woodfall Stray Fairies",    "All 15 collected. Reward: Great Spin Attack.", "", CKI_SPRITE, MSPR_FAIRY, CK_BYTEEQ, 0x7763E8, 0x0F },
    { "fairy_snowhead", "Snowhead Stray Fairies",    "All 15 collected. Reward: Double Magic Meter.", "", CKI_SPRITE, MSPR_FAIRY, CK_BYTEEQ, 0x7763E9, 0x0F },
    { "fairy_greatbay", "Great Bay Stray Fairies",   "All 15 collected. Reward: Enhanced Defense.",                          "", CKI_SPRITE, MSPR_FAIRY, CK_BYTEEQ, 0x7763EA, 0x0F },
    { "fairy_ikana",    "Stone Tower Stray Fairies", "All 15 collected. Reward: Great Fairy's Sword.",                       "", CKI_SPRITE, MSPR_FAIRY, CK_BYTEEQ, 0x7763EB, 0x0F },
    { "fairy_clocktown", "Clock Town Stray Fairy",   "The 61st fairy - Laundry Pool by day, Stock Pot Inn area by night. Reward: Great Fairy's Mask.", "", CKI_SPRITE, MSPR_FAIRY, CK_MANUAL, 0, 0 },
};

// Bosses (4): NOT directly save-file confirmed, but decoded by cross-referencing the zeldaret/mm
// N64 decompilation's `QuestItem` enum (include/z64item.h) against our own CONFIRMED 3-byte
// quest field (0x7763D0-0x7763D2, already used by the "All Bosses and Songs" cheat). That enum's
// 24 entries (0x00-0x17) line up exactly with 24 bits across those 3 bytes - byte0=bits 0-7,
// byte1=bits 8-15, byte2=bits 16-23, LSB first. Decoding the CONFIRMED 100%-save value for that
// field (0xCF 0xF7 0xCF) against the enum: bits 0-3 (ODOLWA/GOHT/GYORG/TWINMOLD) are all 1, which
// is exactly what a 100% save should show - strong evidence the bit order/layout guess is right.
// Still not hardware bit-tested, so flagged accordingly - the byte-level "does this equal
// 0xCF/0xF7/0xCF" version of this cheat IS confirmed, individual bits within it aren't.
static const ChkItem CK_BOSSES[] = {
    { "boss_odolwa",   "Odolwa",   "Woodfall Temple.",   "", CKI_SPRITE, MSPR_BOSS_ODOLWA,   CK_BIT, 0x7763D0, 0x01 },
    { "boss_goht",     "Goht",     "Snowhead Temple.",   "", CKI_SPRITE, MSPR_BOSS_GOHT,     CK_BIT, 0x7763D0, 0x02 },
    { "boss_gyorg",    "Gyorg",    "Great Bay Temple.",  "", CKI_SPRITE, MSPR_BOSS_GYORG,    CK_BIT, 0x7763D0, 0x04 },
    { "boss_twinmold", "Twinmold", "Stone Tower Temple.", "", CKI_SPRITE, MSPR_BOSS_TWINMOLD, CK_BIT, 0x7763D0, 0x08 },
};

// Heart Pieces (52 = 13 extra Heart Containers). No known save address for individual pieces -
// all manual. MM3D swapped exactly one from the N64 original: Koume's Boat-Cruise Target
// Shooting (Swamp Tourist Center) now gives a Bottle instead of a Heart Piece, and the Dampe
// grave-digging game (Ikana Graveyard, Final Night) gives a Heart Piece instead of a Bottle -
// net count (52) is unchanged, but the LOCATION is, so this list reflects the MM3D placement.
static const ChkItem CK_HEARTS[] = {
    // key       task                          hint                                                                 loc icon      arg kind      addr mask
    { "hp_01", "Deku Flower Deed (South Clock Town)", "Trade the Moon's Tear for the Land Title Deed.",             "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_02", "North Clock Town tree climb",   "Climb the tree using blocks to reach the Bunny Hood platform.",    "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_03", "Swordsman's School",            "Clear the Expert Course, West Clock Town.",                        "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_04", "Deku Scrub Playground",         "Win all 3 days, North Clock Town.",                                "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_05", "Post Office timing game",       "East Clock Town Post Office.",                                     "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_06", "Rosa Sisters dance",            "Dance for them at night wearing Kamaro's Mask, West Clock Town.",  "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_07", "Stock Pot Inn toilet hand",     "Trade the Town Title Deed, Final Night only.",                     "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_08", "Keaton Quiz",                   "Wear Keaton Mask, cut every patch of grass first.",                "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_09", "Mailbox check",                 "Wear the Postman's Hat and check a mailbox.",                     "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_10", "Town Shooting Gallery",         "Perfect run, East Clock Town.",                                    "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_11", "Honey & Darling's",             "Win all 3 days, East Clock Town.",                                 "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_12", "Treasure Chest Shop",           "Win as Goron Link, East Clock Town.",                              "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_13", "Anju's Grandmother, story 1",   "Listen to her first story, wearing the All-Night Mask.",           "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_14", "Anju's Grandmother, story 2",   "Listen to her second story, wearing the All-Night Mask.",          "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_15", "Mayor's Residence meeting",     "Wear the Couple's Mask, end the never-ending meeting.",            "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_16", "Clock Town Bank",               "Deposit 5000 rupees total.",                                       "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_17", "Termina Field grotto",          "Underground hole, Termina Field.",                                 "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_18", "Dodongo grotto",                "Defeat all 3 Dodongos, Termina Field.",                            "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_19", "Business Scrub (Termina Field)", "Buy for 100 rupees near the Astral Observatory.",                 "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_20", "Gossip Stones",                 "Play the right songs at all 4 stones, Termina Field.",             "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_21", "Beehive underwater",            "Bomb the boulder, dive as Zora Link, Termina Field.",              "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_22", "Road to Southern Swamp bats",   "Climb up past the bats and vines.",                                "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_23", "Swamp Deku Flower Deed",        "Trade for the Land Title Deed, Swamp Tourist Center.",             "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_24", "Swamp pictograph contest",      "Photo of the Deku King or Tingle, Swamp Tourist Center.",          "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_25", "Deku Palace maze",              "West Garden maze, Deku Palace.",                                   "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_26", "Woodfall Deku Flower ring",     "Hop the ring of flowers, Woodfall.",                               "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_27", "Swamp Shooting Gallery",        "Perfect run, Road to Southern Swamp.",                             "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_28", "Mountain Deku Flower Deed",     "Trade for the Land Title Deed, Goron Village.",                    "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_29", "Road to Snowhead platforms",    "Invisible/ice platforms - use the Lens of Truth.",                 "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_30", "Underwater chest (Goron Village)", "After Goht is defeated and the river thaws.",                   "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_31", "Frog Choir",                    "Reunite all 5 frogs with Don Gero's Mask, Mountain Village.",      "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_32", "Doggy Racetrack",               "Win 150 rupees in one race, Romani Ranch.",                        "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_33", "Ocean Deku Flower Deed",        "Trade for the Land Title Deed, Zora Hall rooftop area.",           "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_34", "Marine Research Lab tank",      "Feed the tank fish until it grows.",                               "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_35", "Like Like (Zora Cape)",         "Kill it as Zora Link.",                                            "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_36", "Pirates' Fortress switch",      "Hidden switch/chest inside the fortress.",                        "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_37", "Seahorse reunion",              "Reunite the seahorses, Pinnacle Rock.",                            "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_38", "Zora Hall jam session",         "Mikau's diary / join Lulu's band, Evan's song.",                   "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_39", "Beaver Race #2",                "Win the second, faster race, Zora Cape.",                          "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_40", "Great Bay Coast bean ledge",    "Hookshot up to the high ledge, plant a Magic Bean.",               "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_41", "Oceanside Spider House secret", "Fireplace secret room chest.",                                     "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_42", "Fisherman's jumping game",      "Score 20+, after clearing Great Bay Temple.",                      "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_43", "Ikana Deku Flower Deed",        "Trade for the Land Title Deed, Ikana Canyon.",                     "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_44", "Iron Knuckle (Graveyard)",      "Wear the Captain's Hat, Day 1 night, Ikana Graveyard.",            "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_45", "Dampe's grave dig",             "Ikana Graveyard, Final Night only.", "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_46", "Poe Sisters",                   "Catch all 4 within the time limit, Beneath the Graveyard.",        "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_47", "Ancient Castle of Ikana roof",  "Rooftop pillar switch.",                                           "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_48", "Secret Shrine",                 "Light Arrow door, defeat all 4 mini-bosses, Ikana Canyon.",        "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_49", "Moon: Deku trial",              "Odolwa's dungeon on the Moon.",                                    "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_50", "Moon: Goron trial",             "Goht's dungeon on the Moon.",                                      "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_51", "Moon: Zora trial",              "Gyorg's dungeon on the Moon.",                                     "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
    { "hp_52", "Moon: Link trial",              "Twinmold's dungeon on the Moon.",                                  "", CKI_SPRITE, MSPR_HEART, CK_MANUAL, 0, 0 },
};

// Songs (13 in MM3D, one more than N64's 12: Song of Storms is new). 11 of 13 auto-detect via
// the same bit-decoded 0x7763D0-0x7763D2 field as CK_BOSSES above (see that comment for the
// decode methodology and confidence caveat) - Inverted Song of Time and Song of Double Time stay
// manual since they're just the Song of Time played differently, not separately-learned songs
// with their own flag. Note colors are a best-effort match to the real in-game Ocarina Songs
// screen's palette (yellow/gold, red, blue, purple, green, orange badges alongside several
// identical cyan ones) - we could not find a source individually labeling which exact song gets
// which color (unlike Masks, where Zelda Wiki had a named icon file per mask), so the per-song
// color assignment is inferred, not confirmed: Sonata of Awakening=yellow, Goron Lullaby=red,
// New Wave Bossa Nova=blue, Elegy of Emptiness=purple, Oath to Order=green, Song of Storms=
// orange, everything else (Time, Healing, Soaring, Epona's, Scarecrow's, both Time variants)
// stays cyan, matching the majority of the real songs screen.
static const ChkItem CK_SONGS[] = {
    { "song_01", "Song of Time",             "Clock Tower, recovered from Skull Kid.",                    "", CKI_NOTE, 0, CK_BIT, 0x7763D1, 0x10 },
    { "song_02", "Song of Healing",          "Clock Tower, from the Happy Mask Salesman.",                "", CKI_NOTE, 0, CK_BIT, 0x7763D1, 0x20 },
    { "song_03", "Inverted Song of Time",    "Play the Song of Time backwards.",                          "", CKI_NOTE, 0, CK_MANUAL, 0, 0 },
    { "song_04", "Song of Double Time",      "Play the Song of Time doubled.",                            "", CKI_NOTE, 0, CK_MANUAL, 0, 0 },
    { "song_05", "Song of Soaring",          "Kaepora Gaebora, Southern Swamp owl statue.",               "", CKI_NOTE, 0, CK_BIT, 0x7763D1, 0x80 },
    { "song_06", "Sonata of Awakening",      "From the monkey, Deku Palace.",                             "", CKI_NOTE, 6, CK_BIT, 0x7763D0, 0x40 },
    { "song_07", "Goron Lullaby",            "Goron Elder + the crying baby, Goron Village.",             "", CKI_NOTE, 2, CK_BIT, 0x7763D0, 0x80 },
    { "song_08", "Epona's Song",             "From Romani, Romani Ranch.",                                "", CKI_NOTE, 0, CK_BIT, 0x7763D1, 0x40 },
    { "song_09", "New Wave Bossa Nova",      "The Zora Eggs, Marine Research Lab.",                       "", CKI_NOTE, 3, CK_BIT, 0x7763D1, 0x01 },
    { "song_10", "Elegy of Emptiness",       "Igos du Ikana, Ancient Castle of Ikana.",                   "", CKI_NOTE, 5, CK_BIT, 0x7763D1, 0x02 },
    { "song_11", "Oath to Order",            "Odolwa's Giant, after clearing Woodfall Temple.",           "", CKI_NOTE, 1, CK_BIT, 0x7763D1, 0x04 },
    { "song_12", "Scarecrow's Song",         "Teach it to Pierre (Trading Post / Astral Observatory).",  "", CKI_NOTE, 0, CK_BIT, 0x7763D2, 0x02 },
    { "song_13", "Song of Storms",           "MM3D-EXCLUSIVE. Beneath the Graveyard, after Flat's eulogy + Iron Knuckle; needs Captain's Hat.", "", CKI_NOTE, 4, CK_BIT, 0x7763D2, 0x01 },
};

// Bomber's Notebook: MM3D overhauled this to 63 trackable events (the N64 original only tracked
// 20 people). Completing all 63 puts a ribbon on the notebook header.
//
// One bit CONFIRMED on hardware: an 8-byte bitfield at 0x777469 flips one bit per notebook event
// (found by diffing all 65 SaveGames/ progressive saves, then verified live - Hex Editor read
// 0x00 at 0x77746D, got Epona at Milk Road, re-read 0x10). The other 62 events still have no
// known bit - most of the field's other single-bit transitions in the save corpus line up with a
// window containing more than one candidate event, so guessing the rest would risk a wrong mark
// nobody would notice. Add more as they get confirmed the same way.
//
// RULED OUT on hardware: bit 55 (byte 0x77746F, mask 0x80) looked promising offline - the only
// bit that changes between save steps "15-3" and "16-1", whose notes say "Frog Choir", matching
// note_47's name exactly - but live-tested it (Hex Editor read 0x59 before AND after completing
// Reunite the Frog Choir) and the byte never changed. The name match was a coincidence; do not
// re-propose this one without a different address.
static const ChkItem CK_NOTEBOOK[] = {
    { "note_01", "A Stay at Stock Pot Inn",       "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_02", "Anju's Anguish",                "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_03", "A Testament of Love",           "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_04", "The Never-Ending Meeting",      "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_05", "Madame Aroma's Search",         "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_06", "A Challenge to Count On",       "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_07", "The Postman's Peril",           "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_08", "Curiosity Shop Rarity",         "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_09", "The Bomb Business",             "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_10", "History of the Carnival",       "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_11", "Termina Mythology",             "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_12", "The Ghost of the Inn",          "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_13", "A Melancholy Melody",           "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_14", "A Dance with Meaning",          "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_15", "Music Moves the Heart",         "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_16", "A Race near Milk Road",         "CONFIRMED on hardware: bit 4 of 0x77746D sets when Epona's Song / Epona is obtained after the Milk Road horseback race with Romani.", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_BIT, 0x77746D, 0x10 },
    { "note_17", "Protect Romani's Cows!",        "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_18", "Protect the Milk!",             "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_19", "Cucco Shack's Cute Chicks",     "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_20", "Find the Stone-Faced Soldier",  "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_21", "Great Fairy of Clock Town",     "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_22", "Great Fairy of the Swamp",      "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_23", "Great Fairy of the Mountains",  "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_24", "Great Fairy of the Ocean",      "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_25", "Great Fairy of the Canyon",     "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_26", "Business Scrub Scramble",       "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_27", "Bank Loyalty Program",          "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_28", "The Suspicious Ocean House",    "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_29", "Target-Shooting Champ",         "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_30", "Swamp Shooting Champ",          "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_31", "Three Days of Gaming",          "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_32", "Lucky Numbers",                 "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_33", "Master Swordsman",              "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_34", "A Treasure-Chest Prize",        "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_35", "Deku Flower Power",             "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_36", "Find a Keaton!",                "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_37", "Secret Gossip",                 "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_38", "Follow That Scrub!",            "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_39", "Pictograph Contest",            "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_40", "The Terrifying Swamp House",    "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_41", "A Potion Hag's New Business",   "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_42", "A Royal Rush",                  "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_43", "A Goron's Grief",               "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_44", "An Explosive Exam",             "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_45", "Goron Races! Rock 'n' Roll!",   "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_46", "A Sharper Sword",               "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_47", "Reunite the Frog Choir",        "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_48", "Win Big at the Doggy Race",     "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_49", "Fishy Friends",                 "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_50", "Spider House Mystery",          "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_51", "A Fish Wish",                   "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_52", "Gimme a Break",                 "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_53", "Race the Beaver Bros.!",        "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_54", "Light It to Right It",          "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_55", "Playing Paparazzi",             "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_56", "A Zora Swan Song",              "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_57", "The Seafarer's Challenge",      "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_58", "Buried Treasure",               "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_59", "Free the Canyon Ghosts",        "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_60", "Vanquished Foes Return",        "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_61", "The Bombers' Code",             "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_62", "Child's Play",                  "", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
    { "note_63", "Fraternal Milk",                "MM3D-EXCLUSIVE quest, also the source of the game's 7th bottle.", "", CKI_SPRITE, MSPR_ALL_ITEMS, CK_MANUAL, 0, 0 },
};

// Owl Statues (10, save points - checked, not slashed, in MM3D, and they save permanently since
// Song of Time no longer erases owl saves here). No known save address - all manual.
static const ChkItem CK_OWLS[] = {
    { "owl_01", "South Clock Town",     "By the Bank.", "", CKI_SPRITE, MSPR_OWL_ICON, CK_MANUAL, 0, 0, 0, MSPR_OWL_MODEL },
    { "owl_02", "Milk Road",            "Termina Field, at the Milk Road entrance.",                    "", CKI_SPRITE, MSPR_OWL_ICON, CK_MANUAL, 0, 0, 0, MSPR_OWL_MODEL },
    { "owl_03", "Southern Swamp",       "Outside the Swamp Tourist Center.",                            "", CKI_SPRITE, MSPR_OWL_ICON, CK_MANUAL, 0, 0, 0, MSPR_OWL_MODEL },
    { "owl_04", "Woodfall",             "In front of Woodfall Temple.",                                 "", CKI_SPRITE, MSPR_OWL_ICON, CK_MANUAL, 0, 0, 0, MSPR_OWL_MODEL },
    { "owl_05", "Mountain Village",     "Near the smithy.",                                             "", CKI_SPRITE, MSPR_OWL_ICON, CK_MANUAL, 0, 0, 0, MSPR_OWL_MODEL },
    { "owl_06", "Snowhead",             "On the path before Snowhead Temple.",                          "", CKI_SPRITE, MSPR_OWL_ICON, CK_MANUAL, 0, 0, 0, MSPR_OWL_MODEL },
    { "owl_07", "Great Bay Coast",      "Near the Marine Research Lab.",                                "", CKI_SPRITE, MSPR_OWL_ICON, CK_MANUAL, 0, 0, 0, MSPR_OWL_MODEL },
    { "owl_08", "Zora Cape",            "Outside Zora Hall.",                                           "", CKI_SPRITE, MSPR_OWL_ICON, CK_MANUAL, 0, 0, 0, MSPR_OWL_MODEL },
    { "owl_09", "Ikana Canyon",         "Atop the cliff past the broken bridge.",                       "", CKI_SPRITE, MSPR_OWL_ICON, CK_MANUAL, 0, 0, 0, MSPR_OWL_MODEL },
    { "owl_10", "Stone Tower",          "At the Stone Tower Temple entrance.",                          "", CKI_SPRITE, MSPR_OWL_ICON, CK_MANUAL, 0, 0, 0, MSPR_OWL_MODEL },
};

// Bottles (7 in MM3D, one more than N64's 6 - two changes net one extra: Koume's Boat-Cruise
// archery gives a Bottle here instead of N64's Heart Piece, AND the Gorman "Fraternal Milk"
// sidequest is entirely new to MM3D). No known save address for "which bottles you've unlocked"
// (as opposed to bottle CONTENTS, which the Bottle #1-7 pickers already read/write) - all manual.
static const ChkItem CK_BOTTLES[] = {
    { "bottle_01", "Kotake's Red Potion",       "Woods of Mystery, after saving Koume.",                              "", CKI_SPRITE, MSPR_B_REDPOTION, CK_MANUAL, 0, 0 },
    { "bottle_02", "Koume's Boat-Cruise",       "Target shooting, 20+ points, Swamp Tourist Center.", "", CKI_SPRITE, MSPR_B_HOTSPRING, CK_MANUAL, 0, 0 }, // playful stand-in: no bottle art fits an archery minigame
    { "bottle_03", "Chateau Romani",            "Survive the alien night with Romani, Romani Ranch.",                 "", CKI_SPRITE, MSPR_B_CHATEAU, CK_MANUAL, 0, 0 },
    { "bottle_04", "Gold Dust",                 "Win the Goron Racetrack after defeating Goht.",                      "", CKI_SPRITE, MSPR_B_GOLDDUST, CK_MANUAL, 0, 0 },
    { "bottle_05", "Fraternal Milk",             "MM3D-EXCLUSIVE. Gorman + Troupe Leader's Mask, fetch within 2 minutes, Stock Pot Inn/Milk Road.", "", CKI_SPRITE, MSPR_B_MILK, CK_MANUAL, 0, 0 },
    { "bottle_06", "Beaver Brothers race #1",   "Win the first race, Zora Cape waterfall cave.",                      "", CKI_SPRITE, MSPR_B_FISH, CK_MANUAL, 0, 0 }, // playful stand-in: a fish for a waterfall-cave swimming race
    { "bottle_07", "Madame Aroma's mail",       "Deliver Priority Mail with Kafei's Mask, Milk Bar.",                 "", CKI_SPRITE, MSPR_BOTTLE, CK_MANUAL, 0, 0 }, // playful stand-in: "message in a bottle" for a mail delivery
};

// Equipment upgrades. The first four already have CONFIRMED cheat addresses elsewhere in this
// plugin (Battle/Inventory folders) and auto-detect from them; the rest have no known address
// and are manual. Great Fairy's Sword is a C-item, not a sword-slot upgrade - tracked here
// separately from the 3 sword tiers. Wallets are 3 tiers total in MM(3D), not 4.
static const ChkItem CK_EQUIP[] = {
    // auto-detected (confirmed addresses)
    { "eq_gilded",   "Gilded Sword",           "Smithy + Gold Dust. Final sword tier.",                    "", CKI_SPRITE, MSPR_SWORD, CK_BYTEEQ,  0x776352, 0x23 },
    { "eq_defense",  "Enhanced Defense",       "Great Bay Stray Fairy reward. Halves damage taken.",       "", CKI_SPRITE, MSPR_DEFENSE, CK_NONZERO, 0x776320, 0x00 },
    { "eq_magic",    "Magic Meter",            "Unlocks the magic bar.",                                   "", CKI_SPRITE, MSPR_MAGIC_FAIRY, CK_NONZERO, 0x77631E, 0x00 },
    { "eq_dmagic",   "Double Magic",           "Snowhead Stray Fairy reward. Doubles magic capacity.", "", CKI_SPRITE, MSPR_MAGIC_FAIRY, CK_NONZERO, 0x77631F, 0x00 },
    // manual (no known address)
    { "eq_razor",    "Razor Sword",            "Mountain Smithy, 100 rupees, wait until morning. Reverts on Song of Time.", "", CKI_SPRITE, MSPR_EQ_RAZOR, CK_MANUAL, 0, 0 },
    { "eq_hero",     "Hero's Shield",          "Starting shield.",                                         "", CKI_SPRITE, MSPR_EQ_HERO, CK_MANUAL, 0, 0 },
    { "eq_mirror",   "Mirror Shield",          "Big chest, Beneath the Well (Ikana).",                     "", CKI_SPRITE, MSPR_DEFENSE, CK_MANUAL, 0, 0 },
    { "eq_gfsword",  "Great Fairy's Sword",    "Stone Tower Stray Fairy reward. A C-item, not a sword upgrade.", "", CKI_SPRITE, MSPR_GFSWORD, CK_MANUAL, 0, 0 },
    { "eq_quiver1",  "Quiver (30)",            "With the Hero's Bow, Woodfall Temple.",                    "", CKI_SPRITE, MSPR_QUIVER, CK_MANUAL, 0, 0 },
    { "eq_quiver2",  "Large Quiver (40)",      "Town Shooting Gallery, 40+ points.",                       "", CKI_SPRITE, MSPR_EQ_QUIVER2, CK_MANUAL, 0, 0 },
    { "eq_quiver3",  "Largest Quiver (50)",    "Swamp Shooting Gallery, perfect run.",                     "", CKI_SPRITE, MSPR_EQ_QUIVER3, CK_MANUAL, 0, 0 },
    { "eq_bombbag1", "Bomb Bag (20)",          "Bomb Shop, 50 rupees.",                                    "", CKI_SPRITE, MSPR_EQ_BOMBBAG1, CK_MANUAL, 0, 0 },
    { "eq_bombbag2", "Big Bomb Bag (30)",      "Bomb Shop, 90 rupees, after saving the old lady Night 1.", "", CKI_SPRITE, MSPR_EQ_BOMBBAG2, CK_MANUAL, 0, 0 },
    { "eq_bombbag3", "Biggest Bomb Bag (40)",  "Goron Village Business Scrub, Big Bomb Bag + 200 rupees.", "", CKI_SPRITE, MSPR_EQ_BOMBBAG3, CK_MANUAL, 0, 0 },
    { "eq_wallet1",  "Adult Wallet (200)",     "Clock Town Bank, deposit 200 rupees.",                     "", CKI_SPRITE, MSPR_WALLET, CK_MANUAL, 0, 0 },
    { "eq_wallet2",  "Giant Wallet (500)",     "Oceanside Spider House, all 30 Gold Skulltulas (any day in MM3D).", "", CKI_SPRITE, MSPR_EQ_WALLET2, CK_MANUAL, 0, 0 },
};

static const ChkCat CHK_CATS[] = {
    { "Masks",            CK_MASKS,    (int)(sizeof(CK_MASKS)    / sizeof(CK_MASKS[0])) },
    { "Heart Pieces",     CK_HEARTS,   (int)(sizeof(CK_HEARTS)   / sizeof(CK_HEARTS[0])) },
    { "Songs",            CK_SONGS,    (int)(sizeof(CK_SONGS)    / sizeof(CK_SONGS[0])) },
    { "Bosses",           CK_BOSSES,   (int)(sizeof(CK_BOSSES)   / sizeof(CK_BOSSES[0])) },
    { "Stray Fairies",    CK_FAIRIES,  (int)(sizeof(CK_FAIRIES)  / sizeof(CK_FAIRIES[0])) },
    { "Bomber's Notebook", CK_NOTEBOOK, (int)(sizeof(CK_NOTEBOOK) / sizeof(CK_NOTEBOOK[0])) },
    { "Owl Statues",       CK_OWLS,    (int)(sizeof(CK_OWLS)     / sizeof(CK_OWLS[0])) },
    { "Bottles",           CK_BOTTLES, (int)(sizeof(CK_BOTTLES)  / sizeof(CK_BOTTLES[0])) },
    { "Equipment",         CK_EQUIP,   (int)(sizeof(CK_EQUIP)    / sizeof(CK_EQUIP[0])) },
};
#define CHK_NCATS  ((int)(sizeof(CHK_CATS) / sizeof(CHK_CATS[0])))
#define CHK_MAXITEMS 63
