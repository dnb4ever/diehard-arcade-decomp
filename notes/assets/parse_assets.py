#!/usr/bin/env python3
"""Die Hard Arcade (ST-V) asset-ROM parser / map dumper.

Byte-analysis of cart.bin. SH-2 addr = cart_off + 0x02000000; stored pointers
often use the uncached mirror 0x22xxxxxx -> normalize (v & 0x07FFFFFF) - 0x02000000.

usage:
  parse_assets.py --map       top-level region map (ff-padding boundaries)
  parse_assets.py --models    region-D 3D model directory @0xd00000
  parse_assets.py --segb      region-B sprite/texture table @0x700000
  parse_assets.py --sound     8 SCSP sound-RAM bank headers @0x1000000
(default: --map)
"""
import struct
import sys
from pathlib import Path

CART = Path(__file__).resolve().parents[2] / "cart.bin"
ROM = CART.read_bytes()

def u8(o):  return ROM[o]
def u16(o): return struct.unpack_from(">H", ROM, o)[0]
def u32(o): return struct.unpack_from(">I", ROM, o)[0]
def s32(o): return struct.unpack_from(">i", ROM, o)[0]
def cart(p): return (p & 0x07FFFFFF) - 0x02000000  # stored ptr -> cart offset

MODEL_DIR = 0xD00000   # region D directory base (record 0 is null)
SEGB_DIR  = 0x700000   # region B table base
SND_BASE  = 0x1000000
SND_STRIDE = 0x80000


def seg_map():
    runs = []
    o = 0x400000
    while o < len(ROM):
        if ROM[o] == 0xFF:
            s = o
            while o < len(ROM) and ROM[o] == 0xFF:
                o += 1
            if o - s >= 0x400:
                runs.append((s, o))
        else:
            o += 1
    print("# region            cart range            used_end   pad")
    prev = 0x400000
    for s, e in runs:
        print("  %08x..%08x  used=%08x  pad=%#x" % (prev, e, s, e - s))
        prev = e


def models(limit=24):
    base = MODEL_DIR + 0x14  # skip null record 0
    print("# idx  vtx face  type        verts@      faces@")
    i = 0
    while i < 1560:
        o = base + i * 0x14
        vc, fc = u8(o), u8(o + 1)
        p1, p2 = u32(o + 4), u32(o + 8)
        if (p1 >> 24) not in (0x02, 0x22):
            break
        t = u32(o + 0x10)
        if i < limit:
            print("  %4d  %3d %3d   %08x  %08x  %08x"
                  % (i + 1, vc, fc, t, cart(p1), cart(p2)))
        i += 1
    print("# total models:", i)


def model(idx):
    """Return (vcount, fcount, verts[list (x,y,z,w)], faces[list (i0..i3, attr bytes)])."""
    o = MODEL_DIR + idx * 0x14
    vc, fc = u8(o), u8(o + 1)
    vp, fp = cart(u32(o + 4)), cart(u32(o + 8))
    verts = [tuple(s32(vp + v * 16 + k * 4) for k in range(4)) for v in range(vc)]
    faces = [(ROM[fp + f * 16], ROM[fp + f * 16 + 1], ROM[fp + f * 16 + 2],
              ROM[fp + f * 16 + 3], ROM[fp + f * 16 + 4:fp + f * 16 + 16])
             for f in range(fc)]
    return vc, fc, verts, faces


def segb(limit=24):
    print("# idx  ptrA@       ptrB@       run_idx  count  cum_off")
    i = 0
    while i < 4096:
        o = SEGB_DIR + i * 0x14
        a = u32(o)
        if (a >> 24) not in (0x02, 0x22):
            break
        if i < limit:
            print("  %4d  %08x    %08x    %6d  %5d  %#x"
                  % (i, cart(a), cart(u32(o + 4)), u32(o + 8), u32(o + 0xC), u32(o + 0x10)))
        i += 1
    print("# total region-B records:", i)


def sound():
    print("# bank  cart base   header(u32,u32)    used_end    used_len")
    for b in range(8):
        base = SND_BASE + b * SND_STRIDE
        end = base + SND_STRIDE
        while end > base and ROM[end - 1] == 0xFF:
            end -= 1
        print("  %4d   %08x   %08x %08x  %08x   %#x"
              % (b, base, u32(base), u32(base + 4), end, end - base))
    print("# driver = base..base+0x4000 (68k) ; samples/seq = +0x4000..+0x70000")


if __name__ == "__main__":
    arg = sys.argv[1] if len(sys.argv) > 1 else "--map"
    {"--map": seg_map, "--models": models, "--segb": segb,
     "--sound": sound}.get(arg, seg_map)()
