#!/usr/bin/env python3
"""
Generates Includes/glyphs.h - the CTRComposer inline button-glyph sheet.

These glyphs are ORIGINAL artwork, drawn procedurally from primitives by this
script. Nothing here is traced, ripped or sampled from any game or sprite sheet:
run the script and you regenerate the entire sheet from the shapes below.

Output format is what DrawGlyph() in Sources/main.c expects:
  - 14x14 pixels, row-major
  - RGBA4444 packed as  v = R4<<12 | G4<<8 | B4<<4 | A4
  - alpha 0 = fully transparent (DrawGlyph skips those pixels)

Packing uses round-to-nearest, i.e. clamp((c + 8) // 17, 0, 15). That is the true
inverse of DrawGlyph's `* 17` decode. Truncating with `c >> 4` instead biases every
channel upward by up to +15/255 and visibly brightens the art.

Usage:  python Assets/gen_glyphs.py            (writes Includes/glyphs.h)
        python Assets/gen_glyphs.py --preview  (also dumps an ASCII preview)

Restyle the buttons by editing FACE/EDGE/MARK below, or the shape helpers.
"""

import os
import sys

N = 14            # glyph size in px (must match `#define GLY` consumed by main.c)
SS = 8            # supersampling factor used to antialias the curved shapes

# Neutral greyscale palette. The glyphs sit on top of whatever the current theme
# paints, so they are deliberately theme-independent: a light face reads on dark
# backgrounds, and the dark outline keeps it legible on light ones.
FACE = (208, 208, 212)   # button face
EDGE = (72, 74, 80)      # outline / bevel
MARK = (40, 41, 46)      # the letter or arrow drawn on the face

# 5x7 letterforms, drawn crisp (not supersampled) so small text stays sharp.
LETTERS = {
    "A": [".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"],
    "B": ["####.", "#...#", "#...#", "####.", "#...#", "#...#", "####."],
    "X": ["#...#", "#...#", ".#.#.", "..#..", ".#.#.", "#...#", "#...#"],
    "Y": ["#...#", "#...#", ".#.#.", "..#..", "..#..", "..#..", "..#.."],
    "L": ["#....", "#....", "#....", "#....", "#....", "#....", "#####"],
    "R": ["####.", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#"],
}


def blank():
    """RGBA float canvas, alpha 0 everywhere."""
    return [[(0.0, 0.0, 0.0, 0.0) for _ in range(N)] for _ in range(N)]


def put(buf, x, y, rgb, a=1.0):
    """Source-over composite of one pixel."""
    if not (0 <= x < N and 0 <= y < N) or a <= 0.0:
        return
    dr, dg, db, da = buf[y][x]
    out_a = a + da * (1.0 - a)
    if out_a <= 0.0:
        buf[y][x] = (0.0, 0.0, 0.0, 0.0)
        return
    sr, sg, sb = rgb
    buf[y][x] = (
        (sr * a + dr * da * (1.0 - a)) / out_a,
        (sg * a + dg * da * (1.0 - a)) / out_a,
        (sb * a + db * da * (1.0 - a)) / out_a,
        out_a,
    )


def shade(buf, coverage_fn):
    """Supersample `coverage_fn(px, py) -> rgb or None` into buf, giving smooth edges."""
    step = 1.0 / SS
    half = step * 0.5
    for y in range(N):
        for x in range(N):
            hits, acc_r, acc_g, acc_b = 0, 0.0, 0.0, 0.0
            for sy in range(SS):
                for sx in range(SS):
                    px = x + sx * step + half
                    py = y + sy * step + half
                    rgb = coverage_fn(px, py)
                    if rgb is not None:
                        hits += 1
                        acc_r += rgb[0]
                        acc_g += rgb[1]
                        acc_b += rgb[2]
            if hits:
                total = SS * SS
                put(buf, x, y, (acc_r / hits, acc_g / hits, acc_b / hits), hits / total)


def stamp_letter(buf, ch, ox, oy):
    """Draw a 5x7 letterform at (ox, oy), crisp."""
    for row, line in enumerate(LETTERS[ch]):
        for col, c in enumerate(line):
            if c == "#":
                put(buf, ox + col, oy + row, MARK, 1.0)


def round_button(ch):
    """A/B/X/Y: a circular face with an outline and a centered letter."""
    buf = blank()
    cx = cy = N / 2.0
    r_out = 6.6
    r_in = 5.3

    def cov(px, py):
        d = ((px - cx) ** 2 + (py - cy) ** 2) ** 0.5
        if d <= r_in:
            return FACE
        if d <= r_out:
            return EDGE
        return None

    shade(buf, cov)
    stamp_letter(buf, ch, (N - 5) // 2, (N - 7) // 2)
    return buf


def shoulder_button(ch):
    """L/R: a wide rounded 'shoulder' slab, letter centered."""
    buf = blank()
    x0, x1 = 0.4, N - 0.4
    y0, y1 = 2.2, N - 2.2
    r = 2.6

    def rounded(px, py, inset):
        ax0, ax1 = x0 + inset, x1 - inset
        ay0, ay1 = y0 + inset, y1 - inset
        rr = max(0.0, r - inset)
        if not (ax0 <= px <= ax1 and ay0 <= py <= ay1):
            return False
        # distance to the rounded corner arcs
        qx = min(max(px, ax0 + rr), ax1 - rr)
        qy = min(max(py, ay0 + rr), ay1 - rr)
        return ((px - qx) ** 2 + (py - qy) ** 2) ** 0.5 <= rr + 1e-9

    def cov(px, py):
        if rounded(px, py, 1.15):
            return FACE
        if rounded(px, py, 0.0):
            return EDGE
        return None

    shade(buf, cov)
    stamp_letter(buf, ch, (N - 5) // 2, (N - 7) // 2)
    return buf


def dpad():
    """D-Pad: a plus/cross with an outline - all axis-aligned, so no AA needed."""
    buf = blank()
    arm = 2          # half-width of an arm
    span = 6         # half-length of an arm
    cx = cy = 7

    def inside(x, y, a, s):
        return (abs(x - cx) <= a and abs(y - cy) <= s) or (abs(y - cy) <= a and abs(x - cx) <= s)

    for y in range(N):
        for x in range(N):
            if inside(x, y, arm, span):
                put(buf, x, y, EDGE, 1.0)
    for y in range(N):
        for x in range(N):
            if inside(x, y, arm - 1, span - 1):
                put(buf, x, y, FACE, 1.0)
    # a small dark hub so the cross reads as a D-Pad rather than a plus sign
    for y in range(cy - 1, cy + 2):
        for x in range(cx - 1, cx + 2):
            put(buf, x, y, MARK, 1.0)
    return buf


def pack(buf):
    """RGBA float canvas -> list of RGBA4444 u16, round-to-nearest."""
    def q(c):
        return min(15, max(0, int((c + 8) // 17)))
    out = []
    for y in range(N):
        for x in range(N):
            r, g, b, a = buf[y][x]
            av = min(15, max(0, int(round(a * 15))))
            if av == 0:
                out.append(0)
            else:
                out.append((q(r) << 12) | (q(g) << 8) | (q(b) << 4) | av)
    return out


def preview(name, buf):
    ramp = " .:-=+*#%@"
    print(f"--- {name}")
    for y in range(N):
        line = ""
        for x in range(N):
            r, g, b, a = buf[y][x]
            if a < 0.15:
                line += " "
            else:
                lum = (0.299 * r + 0.587 * g + 0.114 * b) / 255.0
                line += ramp[min(len(ramp) - 1, int(lum * (len(ramp) - 1)))]
        print("|" + line + "|")


def main():
    order = [
        ("gly_A", "A", round_button("A")),
        ("gly_B", "B", round_button("B")),
        ("gly_X", "X", round_button("X")),
        ("gly_Y", "Y", round_button("Y")),
        ("gly_L", "L", shoulder_button("L")),
        ("gly_R", "R", shoulder_button("R")),
        ("gly_DP", "D-Pad", dpad()),
    ]

    if "--preview" in sys.argv:
        for sym, label, buf in order:
            preview(label, buf)

    here = os.path.dirname(os.path.abspath(__file__))
    out_path = os.path.join(os.path.dirname(here), "Includes", "glyphs.h")

    lines = [
        "#pragma once",
        "// CTRComposer inline button glyphs - ORIGINAL artwork.",
        "//",
        "// AUTO-GENERATED by Assets/gen_glyphs.py - do not hand-edit; edit the generator.",
        "// Drawn procedurally from primitives (circles, rounded slabs, a cross) plus 5x7",
        "// letterforms defined in the script. Nothing is traced or ripped from any game.",
        "//",
        "// RGBA4444, 14x14, row-major, for inline control hints: the text routines scan for",
        "// {A} {B} {X} {Y} {L} {R} {DP} tokens and blit the matching glyph in place.",
        "",
        f"#define GLY {N}",
        "enum { GL_A, GL_B, GL_X, GL_Y, GL_L, GL_R, GL_DP, NUM_GLYPHS };",
        "",
    ]
    for sym, label, buf in order:
        data = pack(buf)
        body = ",".join(str(v) for v in data)
        lines.append(f"static const unsigned short {sym}[{N * N}] = {{{body}}};")
    lines.append("")
    lines.append(
        "static const unsigned short *const glyphs[NUM_GLYPHS] = "
        "{ gly_A,gly_B,gly_X,gly_Y,gly_L,gly_R,gly_DP };"
    )
    lines.append("")

    with open(out_path, "w", newline="\n") as f:
        f.write("\n".join(lines))
    print(f"wrote {out_path}")


if __name__ == "__main__":
    main()
