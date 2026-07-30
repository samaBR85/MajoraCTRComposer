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
