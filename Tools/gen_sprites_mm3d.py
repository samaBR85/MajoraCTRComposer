# Generate includes/sprites.h from the MM3D "Item Icons" (42px grid) and "UI" sheets.
# Format: RGBA4444 (u16), 16x16 for menu rows + 42x42 for a future picker/detail view.
# Same pipeline as OcarinaCTRComposer's Tools/gen_sprites.py, adapted for MM3D: this game's
# sheets don't carry real in-ROM item-byte IDs the way OoT3D's did, so every key here is a
# pseudo-id (0x100+) chosen for this plugin's own cheats, not a game data value.
from PIL import Image

ITEM_SHEET = 'Assets/Sprites/mm3d_item_icons.png'  # 252x1722, 42px grid, ripped by Colbydude
UI_SHEET   = 'Assets/Sprites/mm3d_ui.png'           # 512x300, ripped by xAct
OUT_PATH   = 'RawPlugin/includes/sprites.h'

CELL = 42

# pseudo-id -> ('grid', row, col) cell on the 42px item sheet, or ('px', x0,y0,x1,y1) a raw
# pixel box on the UI sheet (for icons that aren't on the item grid, e.g. the HUD rupee gem).
MAP = {
    0x100: ('grid', 3, 1),          # Arrows - the bow icon (crossed golden arrows)
    0x101: ('grid', 4, 0),          # Bombs
    0x102: ('grid', 4, 1),          # Bombchus
    0x103: ('grid', 4, 2),          # Deku Sticks
    0x104: ('grid', 4, 3),          # Deku Nuts
    0x105: ('grid', 4, 4),          # Magic Beans
    0x106: ('grid', 5, 0),          # Powder Keg
    0x107: ('grid', 24, 2),         # Gilded Sword
    0x108: ('grid', 27, 3),         # Large Quiver
    0x109: ('grid', 32, 2),         # Heart Container - Refill/Max Hearts
    0x10A: ('grid', 8, 4),          # Fairy in a Bottle - playful stand-in for Refill Magic
                                     # (no magic-jar icon on this sheet; the MP bar is a wide
                                     # rectangle that doesn't crop into a square icon cleanly)
    0x10B: ('grid', 24, 4),         # Hero's Shield - playful stand-in for Enhanced Defense
                                     # (no dedicated "defense upgrade" icon exists)
    0x10C: ('grid', 32, 0),         # Bombers' Notebook - HOME folder icon for Quest
    0x10D: ('grid', 18, 0),         # Bunny Hood - the most recognizable MM3D mask, for Have all Masks
    0x10E: ('grid', 3, 0),          # Ocarina - HOME folder icon for Time
    0x10F: ('grid', 35, 3),         # Stray Fairy sprite - for All Stray Fairies
    0x110: ('px', 455, 65, 481, 92),# Red Rupee, cropped from the UI sheet's HUD - Max Rupees
    # HOME folder icons + reassigned cheat icons (freed up when their sprite moved to a folder)
    0x111: ('grid', 28, 0),         # Rupee Wallet - HOME folder icon for Inventory
    0x112: ('grid', 5, 1),          # Pictograph Box (camera) - playful HOME folder icon for Misc
    0x113: ('grid', 14, 1),         # Trade Quest deed/scroll - playful stand-in for Have all Items
    0x114: ('grid', 31, 0),         # Odolwa's Remains - stand-in for All Bosses and Songs
    # Tools / Settings icons (engine-level entries, no game address behind them - picked for
    # thematic fit like everything else "playful": Cheat Search's own aesthetic is a lens,
    # RAM Dumper "maps" memory, Hex Editor "pinpoints" a byte, a mask changes your look like a
    # theme changes the menu's, a written scroll represents picking a language).
    # NOTE: 0x115 is a best-effort ID - the sheet doesn't clearly label this item, it reads as
    # either the Hookshot or a similar upward-reaching tool. Fix the cell if it turns out wrong.
    0x115: ('grid', 5, 3),          # (tentative) Hookshot - Moon Jump (no boot icon exists on this sheet)
    0x116: ('grid', 5, 2),          # Lens of Truth - Cheat Search
    0x117: ('grid', 35, 2),         # Dungeon Map - RAM Dumper
    0x118: ('grid', 35, 1),         # Compass - Hex Editor
    0x119: ('grid', 21, 2),         # Garo's Mask - Change Theme
    0x11A: ('grid', 14, 2),         # Trade Quest scroll (green) - Language
    # About icon: not on the 42px item grid - cropped separately (see gen_logo.py's sibling
    # crop of the same HOME banner sheet) to a pre-made 48x48 PNG.
    0x11B: ('file', 'Assets/Sprites/mm3d_about_icon.png'),
    0x11C: ('grid', 8, 0),           # empty Bottle - Bottle #1-7 pickers
    0x11D: ('grid', 24, 3),          # Fishing Rod - Fishing Hole Pass
    0x11E: ('grid', 24, 1),          # Great Fairy's Sword (plain silver blade) - B Button picker
    # Per-option icons for the Bottle #1-7 picker (Bottles row on the item sheet, r8-11). Content
    # values (0x12-0x27) come from the AR code list; the row-major sheet order lines up with that
    # same ascending id order, with a couple of gaps where the sheet has no distinct art (those
    # ids reuse a neighboring bottle's icon below instead of guessing new art).
    0x120: ('grid', 8, 1),           # Red Potion
    0x121: ('grid', 8, 2),           # Green Potion
    0x122: ('grid', 8, 3),           # Blue Potion
    0x123: ('grid', 8, 4),           # Fairy
    0x124: ('grid', 8, 5),           # Deku Princess
    0x125: ('grid', 9, 0),           # Milk
    0x126: ('grid', 9, 2),           # Fish
    0x127: ('grid', 9, 4),           # Bug
    0x128: ('grid', 9, 5),           # Big Poe
    0x129: ('grid', 10, 0),          # Spring Water
    0x12A: ('grid', 10, 1),          # Hot Spring Water
    0x12B: ('grid', 10, 3),          # Gold Dust
    0x12C: ('grid', 10, 4),          # Magic Mushroom
    0x12D: ('grid', 10, 5),          # Sea Horse
    0x12E: ('grid', 11, 0),          # Chateau Romani
    0x12F: ('grid', 9, 1),           # Milk (Half) (also reused for nothing else now - see main.c)
    0x130: ('grid', 20, 5),          # Zora Mask - Play as... folder
    0x131: ('grid', 21, 5),          # Fierce Deity's Mask - Play as... folder
    0x132: ('grid', 10, 2),          # Zora Egg (playful stand-in - a round glowing orb)
    # 100% Checklist icons (Heart Pieces category reuses the existing MSPR_HEART, 0x109)
    0x134: ('grid', 24, 0),          # Razor Sword (playful stand-in - the sheet's 3rd sword cell)
    0x135: ('grid', 24, 5),          # Hero's Shield (round plain shield - Mirror Shield reuses the existing MSPR_DEFENSE, same cell 24,4)
    0x136: ('grid', 27, 0),          # Bomb Bag (tier 1)
    0x137: ('grid', 27, 1),          # Big Bomb Bag (tier 2)
    0x138: ('grid', 27, 2),          # Biggest Bomb Bag (tier 3)
    0x139: ('grid', 27, 4),          # Large Quiver (tier 2)
    0x13A: ('grid', 27, 5),          # Largest Quiver (tier 3)
    0x13B: ('grid', 28, 1),          # Giant Wallet (tier 2, red-gem pouch)
    # Owl Statue: a real in-game icon (12x16, The Spriters Resource / Zelda wiki asset) for list
    # rows, and a separate bigger 3D-render crop for the Checklist's detail card - see the
    # `iconArgBig` field on ChkItem in main.c for how the two get selected per cell size.
    0x13C: ('file', 'Assets/Sprites/mm3d_owl_icon.png'),
    0x13D: ('file', 'Assets/Sprites/mm3d_owl_model.png'),
    # Individual Mask icons for the 100% Checklist (one real sprite per mask instead of one
    # shared "masks" icon). 22 of 24 are the real MM3D icon from Zelda Wiki (zeldawiki.wiki,
    # cdn.wikimg.net) at their native 42x42 - Bunny Hood and Keaton Mask both redirected to an
    # unrelated shared icon there (a wiki data issue, not a game asset), so those two are cropped
    # from our own sheet instead (rows 19,2 and 19,0 - see the Masks-section grid analysis).
    0x140: ('file', 'Assets/Sprites/masks/deku.png'),
    0x141: ('file', 'Assets/Sprites/masks/goron.png'),
    0x142: ('file', 'Assets/Sprites/masks/truth.png'),
    0x143: ('file', 'Assets/Sprites/masks/kafei.png'),
    0x144: ('file', 'Assets/Sprites/masks/allnight.png'),
    0x145: ('file', 'Assets/Sprites/masks/bunny.png'),
    0x146: ('file', 'Assets/Sprites/masks/keaton.png'),
    0x147: ('file', 'Assets/Sprites/masks/romani.png'),
    0x148: ('file', 'Assets/Sprites/masks/troupe.png'),
    0x149: ('file', 'Assets/Sprites/masks/postman.png'),
    0x14A: ('file', 'Assets/Sprites/masks/couple.png'),
    0x14B: ('file', 'Assets/Sprites/masks/greatfairy.png'),
    0x14C: ('file', 'Assets/Sprites/masks/gibdo.png'),
    0x14D: ('file', 'Assets/Sprites/masks/dongero.png'),
    0x14E: ('file', 'Assets/Sprites/masks/kamaro.png'),
    0x14F: ('file', 'Assets/Sprites/masks/captain.png'),
    0x150: ('file', 'Assets/Sprites/masks/stone.png'),
    0x151: ('file', 'Assets/Sprites/masks/bremen.png'),
    0x152: ('file', 'Assets/Sprites/masks/blast.png'),
    0x153: ('file', 'Assets/Sprites/masks/scents.png'),
    0x154: ('file', 'Assets/Sprites/masks/giant.png'),
    # Individual Boss Remains icons for the 100% Checklist (one real sprite per boss instead of
    # the shared Odolwa's Remains placeholder at 0x114). All 4 from Zelda Wiki (zeldawiki.wiki,
    # cdn.wikimg.net), native size.
    0x155: ('file', 'Assets/Sprites/bosses/odolwa.png'),
    0x156: ('file', 'Assets/Sprites/bosses/goht.png'),
    0x157: ('file', 'Assets/Sprites/bosses/gyorg.png'),
    0x158: ('file', 'Assets/Sprites/bosses/twinmold.png'),
    # Deku/Goron/Zora/Fierce Deity's masks are shared with the Misc "Play as..." rows and the
    # Bottle/B Button pickers where applicable - Zora and Fierce Deity already have real sprites
    # (MSPR_ZORA_MASK/MSPR_FD_MASK, from this same sheet); Deku/Goron get their own here since
    # nothing else in the plugin needed them yet.
}

def to4444(im):
    out = []
    for (r, g, b, a) in im.getdata():
        out.append(((r >> 4) << 12) | ((g >> 4) << 8) | ((b >> 4) << 4) | (a >> 4))
    return out

item_sheet = Image.open(ITEM_SHEET).convert('RGBA')
ui_sheet = Image.open(UI_SHEET).convert('RGBA')
keys = sorted(MAP.keys())

with open(OUT_PATH, 'w') as f:
    f.write("#pragma once\n")
    f.write("// MM3D icons: Item Icons ripped by Colbydude, UI (rupee) ripped by xAct - both\n")
    f.write("// from The Spriters Resource, https://www.spriters-resource.com/3ds/thelegendofzeldamajorasmask3d/\n")
    f.write("// Converted to RGBA4444 by Tools/gen_sprites_mm3d.py. Keys are plugin-defined pseudo-ids\n")
    f.write("// (this game's sheets don't carry real in-ROM item-byte IDs the way OoT3D's did).\n\n")
    f.write("#define SPR16 16\n#define SPR42 42\n\n")

    for k in keys:
        spec = MAP[k]
        if spec[0] == 'grid':
            _, r, c = spec
            cell = item_sheet.crop((c * CELL, r * CELL, (c + 1) * CELL, (r + 1) * CELL))
        elif spec[0] == 'file':
            cell = Image.open(spec[1]).convert('RGBA').resize((CELL, CELL), Image.LANCZOS)
        else:
            _, x0, y0, x1, y1 = spec
            cell = ui_sheet.crop((x0, y0, x1, y1)).resize((CELL, CELL), Image.LANCZOS)
        i16 = cell.resize((16, 16), Image.LANCZOS)
        for name, im in ((f"spr16_{k:03X}", i16), (f"spr42_{k:03X}", cell)):
            data = to4444(im)
            f.write(f"static const unsigned short {name}[{len(data)}] = {{")
            f.write(','.join(str(v) for v in data))
            f.write("};\n")

    f.write("\ntypedef struct { unsigned short key; const unsigned short *px16; const unsigned short *px42; } SpriteRef;\n")
    f.write(f"#define NUM_SPRITES {len(keys)}\n")
    f.write("static const SpriteRef sprites[NUM_SPRITES] = {\n")
    for k in keys:
        f.write(f"    {{ 0x{k:03X}, spr16_{k:03X}, spr42_{k:03X} }},\n")
    f.write("};\n")

print('sprites.h written,', len(keys), 'sprites')
