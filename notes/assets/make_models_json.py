#!/usr/bin/env python3
"""Export a curated set of Die Hard Arcade (ST-V) region-D 3D models to one
compact JSON for the browser viewer. See notes/assets/README.md §9.

Vertex coords are 16.16 fixed-point (proven: box model 4 -> exact +-0.5/1.0),
so float = raw / 65536. Per-face colour index = face record last u16 (+0x0e)
-> emitted as "cidx" (raw palette index; true RGB CLUT still unlocated, §6).

usage: make_models_json.py  [out.json]   (default: models.json beside this file)
"""
import struct, json, sys
from pathlib import Path

CART = Path(__file__).resolve().parents[2] / "cart.bin"
ROM = CART.read_bytes()
MODEL_DIR = 0xD00000
SCALE = 65536.0

def u8(o):  return ROM[o]
def u16(o): return struct.unpack_from(">H", ROM, o)[0]
def u32(o): return struct.unpack_from(">I", ROM, o)[0]
def s32(o): return struct.unpack_from(">i", ROM, o)[0]
def cart(p): return (p & 0x07FFFFFF) - 0x02000000

def rec(idx):
    o = MODEL_DIR + idx * 0x14
    return u8(o), u8(o + 1), cart(u32(o + 4)), cart(u32(o + 8)), u32(o + 0x10)

def curate(n=48):
    allm = []
    for idx in range(1, 1561):
        vc, fc, *_ = rec(idx)
        if vc and fc:
            allm.append((idx, vc, fc))
    allm.sort(key=lambda x: x[2])            # spread across face count
    picks = {2, 3, 4, 6, 30, 37, 64}         # clean geometric primitives
    for i in range(n):
        picks.add(allm[int(i * (len(allm) - 1) / (n - 1))][0])
    return sorted(picks)

def main():
    out = []
    for idx in curate():
        vc, fc, vp, fp, _t = rec(idx)
        verts = [[round(s32(vp + v * 16 + k * 4) / SCALE, 4) for k in range(3)]
                 for v in range(vc)]
        faces, cidx = [], []
        for f in range(fc):
            b = fp + f * 16
            faces.append([ROM[b], ROM[b + 1], ROM[b + 2], ROM[b + 3]])
            cidx.append(u16(b + 14))
        out.append({"id": idx, "verts": verts, "faces": faces, "cidx": cidx})
    dest = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).with_name("models.json")
    dest.write_text(json.dumps(out, separators=(",", ":")))
    print("wrote %s  (%d models, %.1f KB)" % (dest, len(out), dest.stat().st_size / 1024))

if __name__ == "__main__":
    main()
