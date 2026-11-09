#!/usr/bin/env python3
"""Extract the Die Hard Arcade (ST-V) 3D-model master colour palette (CRAM CLUT).

The region-D flat-shaded models carry a per-face colour INDEX (cidx = face+0x0e,
bit15=0).  cidx is a *global* index into one contiguous RGB555 master palette that
lives in region B of the cart at 0x9bb438 (A-bus SH2 addr 0x229bb438).

Format: Saturn BGR555, 16-bit big-endian, one colour per u16:
        bit15 = 0 (unused/msb), bits14..10 = Blue, 9..5 = Green, 4..0 = Red.
0xffff entries are white sentinels (mask to 0x7fff = white).

5-bit -> 8-bit via bit replication: (c<<3)|(c>>2).

Writes palette.json: {meta..., "palette":[[r,g,b],...]} where array index == cidx.
"""
import json, struct
from pathlib import Path

CART = Path(__file__).resolve().parents[2] / "cart.bin"
ROM  = CART.read_bytes()
BASE = 0x9bb438          # cart offset of master model palette (SH2 0x229bb438)
NENT = 0x18ac            # entries 0..0x18ab inclusive (covers every model cidx)

def u16(o):  return struct.unpack(">H", ROM[o:o+2])[0]
def exp5(c): return (c << 3) | (c >> 2)

pal = []
for c in range(NENT):
    v = u16(BASE + c * 2) & 0x7fff      # drop bit15 (sentinel/msb)
    b = (v >> 10) & 31
    g = (v >> 5)  & 31
    r =  v        & 31
    pal.append([exp5(r), exp5(g), exp5(b)])

out = {
    "format": "Saturn BGR555 (0b_bbbbb_ggggg_rrrrr), 16-bit big-endian, bit15=0",
    "rom_cart_offset": "0x9bb438",
    "sh2_address": "0x229bb438",
    "cram_load": "256-colour sub-CLUTs DMAd to VDP2 CRAM 0x25f00000 per scene "
                 "(pointer table @cart 0x700aa0, transfer-type 14 via enqueue @SH2 0x0021632e)",
    "index_meaning": "cidx (face record +0x0e) is a direct global index into this array",
    "entry_count": NENT,
    "expand_5to8": "(c<<3)|(c>>2)",
    "palette": pal,
}
dst = Path(__file__).with_name("palette.json")
dst.write_text(json.dumps(out, separators=(",", ":")))
print("wrote", dst, "entries", NENT, "bytes", dst.stat().st_size)
