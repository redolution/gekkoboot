#!/usr/bin/env python3

# Trivial script to inject a BIOS image into a Qoob updater

import sys

bios = sys.argv[1]
updater = sys.argv[2]
outfile = sys.argv[3]

with open(bios, "rb") as f:
    img = f.read()

with open(updater, "rb") as f:
    out = bytearray(f.read())

if bios.endswith(".gcb"):
    if len(img) > 128 * 1024:
        raise "Qoob Pro BIOS image too big to fit in flasher"

    msg = b"QOOB Pro iplboot install\0"
    msg_offset = 0x1A68
    img_offset = 0x1AE0

if bios.endswith(".qbsx"):
    if len(img) > 62800:
        raise "Qoob SX BIOS image too big to fit in flasher"

    msg = b"QOOB SX iplboot install\0"
    msg_offset = 7240
    img_offset = 7404

out[msg_offset:msg_offset + len(msg)] = msg
out[img_offset:img_offset + len(img)] = img

with open(outfile, "wb") as f:
    f.write(out)
