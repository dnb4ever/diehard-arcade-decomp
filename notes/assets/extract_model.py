#!/usr/bin/env python3
"""Export a Die Hard Arcade (ST-V) region-D 3D model to Wavefront OBJ.

The region-D directory @cart 0xd00000 holds 1560 models: per record,
vertex_count/face_count + pointers to a vertex array (4x s32 X,Y,Z,W per vertex)
and a face array (4 vertex indices per quad + 12 attr bytes).

usage: extract_model.py <model_index 1..1560> [out.obj]
       extract_model.py <idx> -    # print to stdout
Faces are emitted as quads (degenerate quads collapse to tris automatically in
most tools). W (4th vertex word) is ignored; flag it if you find it non-zero.
"""
import sys
from parse_assets import model  # same directory


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return
    idx = int(sys.argv[1], 0)
    out = sys.argv[2] if len(sys.argv) > 2 else "model_%04d.obj" % idx
    vc, fc, verts, faces = model(idx)
    lines = ["# Die Hard Arcade ST-V model %d  (%d verts, %d faces)" % (idx, vc, fc)]
    nonzero_w = sum(1 for v in verts if v[3] != 0)
    if nonzero_w:
        lines.append("# NOTE: %d/%d verts have non-zero 4th word (W) — investigate" % (nonzero_w, vc))
    for x, y, z, _w in verts:
        lines.append("v %d %d %d" % (x, y, z))
    for a, b, c, d, attr in faces:
        # OBJ is 1-indexed; collapse degenerate quad (repeated index) to a tri
        q = [a, b, c, d]
        uniq = list(dict.fromkeys(q))
        idxs = uniq if len(uniq) < 4 else q
        lines.append("f " + " ".join(str(i + 1) for i in idxs) +
                     "   # attr=" + attr.hex())
    text = "\n".join(lines) + "\n"
    if out == "-":
        sys.stdout.write(text)
    else:
        with open(out, "w") as f:
            f.write(text)
        print("wrote %s  (%d v, %d f)" % (out, vc, fc))


if __name__ == "__main__":
    main()
