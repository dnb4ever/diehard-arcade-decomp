#!/usr/bin/env python3
"""Export ALL 1560 Die Hard Arcade (ST-V) region-D models to one compact JSON
for the in-browser library viewer. See notes/assets/README.md section 1 & 11.

Format (models_all.json):
  { "scale": 65536, "count": 1560,
    "models": [ {"id":N,"v":[[x,y,z],...],"f":[[i,j,k,l],...],"c":[cidx,...]},...] }
Vertices: raw s32 16.16 / 65536, rounded to 3 dp (W 4th word dropped; flagged in
"wflag" count if any model has non-zero W). Faces: 4 quad vertex indices
(degenerate quad = repeated index = triangle). c: per-face colour index
(face record +0x0e), a direct index into palette.json. Malformed records are
skipped and listed under "skipped".

usage: make_models_all.py [out.json]   (default: models_all.json beside this)
"""
import struct, json, sys
from pathlib import Path

CART = Path(__file__).resolve().parents[2] / "cart.bin"
ROM  = CART.read_bytes()
N    = len(ROM)
MODEL_DIR = 0xD00000
SCALE = 65536.0

def u8(o):  return ROM[o]
def u16(o): return struct.unpack_from(">H", ROM, o)[0]
def u32(o): return struct.unpack_from(">I", ROM, o)[0]
def s32(o): return struct.unpack_from(">i", ROM, o)[0]
def cart(p): return (p & 0x07FFFFFF) - 0x02000000

def main():
    models, skipped, wflag = [], [], 0
    for idx in range(1, 1561):
        o = MODEL_DIR + idx * 0x14
        vc, fc = u8(o), u8(o + 1)
        p1, p2 = u32(o + 4), u32(o + 8)
        if (p1 >> 24) not in (0x02, 0x22) or (p2 >> 24) not in (0x02, 0x22):
            skipped.append([idx, "badptr"]); continue
        vp, fp = cart(p1), cart(p2)
        if vc == 0 or fc == 0:
            skipped.append([idx, "zerocount"]); continue
        if vp < 0 or fp < 0 or vp + vc*16 > N or fp + fc*16 > N:
            skipped.append([idx, "range"]); continue
        verts, bad = [], False
        for v in range(vc):
            b = vp + v*16
            if s32(b+12) != 0: wflag += 1
            verts.append([round(s32(b)/SCALE,3), round(s32(b+4)/SCALE,3), round(s32(b+8)/SCALE,3)])
        faces, cidx = [], []
        for f in range(fc):
            b = fp + f*16
            q = [ROM[b], ROM[b+1], ROM[b+2], ROM[b+3]]
            if any(i >= vc for i in q):
                bad = True; break
            faces.append(q)
            cidx.append(u16(b+14))
        if bad:
            skipped.append([idx, "oobface"]); continue
        models.append({"id": idx, "v": verts, "f": faces, "c": cidx})
    doc = {"scale": 65536, "count": len(models), "wflag": wflag,
           "skipped": skipped, "models": models}
    dest = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).with_name("models_all.json")
    dest.write_text(json.dumps(doc, separators=(",", ":")))
    print("wrote %s  (%d models, %d skipped, %d verts with W!=0, %.2f MB)"
          % (dest, len(models), len(skipped), wflag, dest.stat().st_size/1048576))

if __name__ == "__main__":
    main()
