#!/usr/bin/env python3
"""Decode Die Hard Arcade (ST-V) region-C animation: the clip directory + the
0x6c-byte frame-record stream. See notes/assets/anim/README.md.

Outputs (beside this script):
  clips_index.json  - all 1236 clips: id, cart offset of frame data, keyframe
                      count, loop/type/sub flags, and non-zero event cues.
  sample_anim.json  - ONE clip's full per-frame data (54 s16 per frame) so a
                      viewer can play it back.

Directory lives in the program/data image; the region-C pointer column is at
cart 0x27913c, stride 0x1c, 1236 records. Record base = ptrcol-0x18:
  +0x08 u32 flag   +0x0c u32 event-cue (packed, usu. 0)
  +0x10 u16 type   +0x12 u16 sub
  +0x14 u16 KEYFRAME COUNT   +0x16 u16 loop/flag
  +0x18 u32 ptr -> region-C frame data (advanced +0x6c per frame by the engine)
Frame record = 0x6c bytes = 54x s16 BE (see README for field analysis).
"""
import struct, json, sys
from pathlib import Path

CART = Path(__file__).resolve().parents[3] / "cart.bin"
ROM = CART.read_bytes()
def u16(o): return struct.unpack_from(">H", ROM, o)[0]
def u32(o): return struct.unpack_from(">I", ROM, o)[0]
def s16(o): return struct.unpack_from(">h", ROM, o)[0]
def cart(p): return (p & 0x07FFFFFF) - 0x02000000

N = 1236
PTRCOL = 0x27913c
C_LO, C_HI = 0xA00000, 0xCE83D8

def record(i):
    pcol = PTRCOL + i * 0x1c
    base = pcol - 0x18
    return {"id": i, "off": cart(u32(pcol)), "kf": u16(base+0x14),
            "loop": u16(base+0x16), "type": u16(base+0x10), "sub": u16(base+0x12),
            "flag": u32(base+0x08), "cue": u32(base+0x0c)}

def main():
    clips = [record(i) for i in range(N)]
    # compact index: drop zero flag/cue to save space
    idx = []
    for c in clips:
        e = {"id": c["id"], "off": c["off"], "kf": c["kf"],
             "loop": c["loop"], "type": c["type"], "sub": c["sub"]}
        if c["flag"]: e["flag"] = c["flag"]
        if c["cue"]:  e["cue"]  = c["cue"]
        idx.append(e)
    doc = {"count": N, "stride": 0x6c, "shorts_per_frame": 54,
           "region_c": [C_LO, C_HI], "clips": idx}
    Path(__file__).with_name("clips_index.json").write_text(json.dumps(doc, separators=(",", ":")))

    # sample: a clean short LOOPING clip fully inside region C
    sample = next(c for c in clips if c["id"] == 9)
    off, kf = sample["off"], sample["kf"]
    frames = [[s16(off + f*0x6c + i*2) for i in range(54)] for f in range(kf)]
    sdoc = {"clip_id": sample["id"], "cart_offset": off, "keyframes": kf,
            "loop": sample["loop"], "type": sample["type"], "sub": sample["sub"],
            "stride_bytes": 0x6c, "shorts_per_frame": 54,
            "model_id": None,
            "note": ("Raw per-frame pose stream (54 s16/frame). Model association "
                     "is NOT stored in the clip directory - it comes from the actor "
                     "that plays the clip (entity+0x5a=clip id, entity carries its "
                     "region-D model). Clip->model link unresolved; see README."),
            "frames": frames}
    Path(__file__).with_name("sample_anim.json").write_text(json.dumps(sdoc, separators=(",", ":")))

    ic = Path(__file__).with_name("clips_index.json")
    sc = Path(__file__).with_name("sample_anim.json")
    tf = sum(c["kf"] for c in clips)
    print("clips_index.json %.1f KB (%d clips, %d total keyframes, %d bytes vs region-C %d)"
          % (ic.stat().st_size/1024, N, tf, tf*0x6c, C_HI-C_LO))
    print("sample_anim.json %.1f KB (clip %d, %d frames)"
          % (sc.stat().st_size/1024, sample["id"], kf))

if __name__ == "__main__":
    main()
