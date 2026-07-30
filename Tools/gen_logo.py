# Generate includes/logo.h from the MM3D title wordmark (About screen header).
# Source: the "HOME Menu Icons and Banners" sheet (Hiccup / Cheesy Mac n Cheese, The Spriters
# Resource) - cropped to the English "THE LEGEND OF ZELDA: MAJORA'S MASK 3D" banner, a stray
# 1px divider line matted out, then resized to fit the plugin window width. RGBA4444, alpha
# already transparent around the text in the source (no extra matting needed).
from PIL import Image

SRC = 'Assets/Sprites/mm3d_logo_final.png'
OUT = 'RawPlugin/includes/logo.h'

im = Image.open(SRC).convert('RGBA')
w, h = im.size

def to4444(pix):
    r, g, b, a = pix
    return ((r >> 4) << 12) | ((g >> 4) << 8) | ((b >> 4) << 4) | (a >> 4)

vals = [to4444(p) for p in im.getdata()]

with open(OUT, 'w') as f:
    f.write("#pragma once\n")
    f.write("// MM3D title wordmark (About screen header), ripped by Hiccup / Cheesy Mac n Cheese\n")
    f.write("// - The Spriters Resource. RGBA4444.\n")
    f.write(f"#define LOGO_W {w}\n#define LOGO_H {h}\n")
    f.write(f"static const unsigned short logoPx[{w*h}] = {{\n")
    for i in range(0, len(vals), 16):
        f.write(','.join(str(v) for v in vals[i:i+16]) + ',\n')
    f.write("};\n")

print('logo.h written', w, h)
