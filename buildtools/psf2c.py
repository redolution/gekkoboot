#!/usr/bin/env python3

import sys

infile = sys.argv[1]
outfile = sys.argv[2]
symbol = sys.argv[3]

with open(infile, "rb") as f:
    data = f.read()

# Magic word: PSF v1
assert data[:2] == b"\x36\x04"

flags = data[2]
height = data[3]

PSF1_MODE512 = 1 << 0
PSF1_MODEHASTAB = 1 << 1
PSF1_MODESEQ = 1 << 2
chars = 512 if flags & PSF1_MODE512 else 256
has_table = bool(flags & (PSF1_MODEHASTAB | PSF1_MODESEQ))

# Constrain the font a little
assert chars == 256
assert height == 16

data = data[4:]
glyphs = data[:chars * height]
directory = data[chars * height:]

with open(outfile, "w") as f:
    f.write(f"unsigned char {symbol}[] = {{\n")
    while glyphs:
        glyph = glyphs[:height]
        glyphs = glyphs[height:]
        f.write(
            "    "
            + " ".join(f"{b:#04x}," for b in glyph)
            + "\n"
        )
    f.write("};\n")
