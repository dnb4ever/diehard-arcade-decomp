#!/usr/bin/env python3
"""recolor.py  v1.00 -- Die Hard Arcade (Sega ST-V, set 'diehard') RECOLOR mod.

Patches the 3D-model master colour palette and repackages a MAME-loadable romset.

Facts (see notes/assets/README.md section 6/10, make_palette.py, build_cart.py):
  * The flat-shaded region-D models carry a per-face colour INDEX (cidx).
  * cidx indexes ONE contiguous RGB/BGR555 master palette living in region B.
  * In the reconstructed cart.bin that palette is at cart offset 0x9bb438
    (SH-2 0x229bb438), 0x18ac (6316) entries, 16-bit big-endian Saturn BGR555:
        bit15 = unused/white-sentinel, bits14..10 = Blue, 9..5 = Green, 4..0 = Red.

How the palette maps back to a romset file (reverse of build_cart.py):
  build_cart.py places mpr19116.3 at cart 0x800000 via ROM_LOAD16_WORD_SWAP,
  i.e.  cart[0x800000:] = word_swap(mpr19116.3)  where word_swap swaps the two
  bytes of every 16-bit word.  word_swap is its own inverse.  Therefore the
  palette lives inside mpr19116.3 at offset 0x9bb438 - 0x800000 = 0x1bb438, and
  to write edited colours back:  mpr_region = word_swap(new_cart_region).

Usage:
  recolor.py --in diehard.zip --out mod/diehard_recolor.zip --mode hue --hue 0.5
  recolor.py --in diehard.zip --out mod/diehard_recolor.zip --mode swap_rb
Modes: hue (rotate hue by --hue, 0..1), swap_rb (swap red/blue), tint (--tint r,g,b
  multipliers), invert.  Or import recolor_palette() and pass your own function.
"""
import argparse, colorsys, struct, zipfile
from pathlib import Path

MPR_FILE   = "mpr19116.3"     # romset file that holds the palette
PAL_CART   = 0x9bb438         # cart.bin offset of the master palette
MPR_PLACE  = 0x800000         # where build_cart.py places mpr19116.3 in cart.bin
PAL_IN_MPR = PAL_CART - MPR_PLACE   # 0x1bb438  -- offset inside mpr19116.3
NENT       = 0x18ac           # 6316 entries
PAL_BYTES  = NENT * 2         # 0x3158 bytes


def word_swap(b: bytes) -> bytes:
    """Swap the two bytes of every 16-bit word (identical to build_cart.py)."""
    b = bytearray(b)
    b[0::2], b[1::2] = b[1::2], b[0::2]
    return bytes(b)


def read_palette(mpr: bytes):
    """Return the NENT big-endian BGR555 u16 values, in cart (display) order."""
    cart_region = word_swap(mpr[PAL_IN_MPR:PAL_IN_MPR + PAL_BYTES])
    return list(struct.unpack(f">{NENT}H", cart_region))


def write_palette(mpr: bytes, vals) -> bytes:
    """Splice edited u16 values back into a full mpr19116.3 image."""
    assert len(vals) == NENT
    cart_region = struct.pack(f">{NENT}H", *(v & 0xFFFF for v in vals))
    return mpr[:PAL_IN_MPR] + word_swap(cart_region) + mpr[PAL_IN_MPR + PAL_BYTES:]


# ---- colour helpers (5-bit BGR555 u16 <-> float RGB) -----------------------
def unpack555(v):
    b = (v >> 10) & 31; g = (v >> 5) & 31; r = v & 31
    return r / 31.0, g / 31.0, b / 31.0

def pack555(r, g, b):
    R = max(0, min(31, round(r * 31)))
    G = max(0, min(31, round(g * 31)))
    B = max(0, min(31, round(b * 31)))
    return (B << 10) | (G << 5) | R    # bit15 stays 0


# ---- built-in transforms ---------------------------------------------------
def make_transform(mode, hue=0.5, tint=(1.0, 1.0, 1.0)):
    def xf(v):
        if v & 0x8000:          # white sentinel / bit15 set -> leave untouched
            return v
        r, g, b = unpack555(v)
        if mode == "hue":
            h, s, val = colorsys.rgb_to_hsv(r, g, b)
            h = (h + hue) % 1.0
            r, g, b = colorsys.hsv_to_rgb(h, s, val)
        elif mode == "swap_rb":
            r, b = b, r
        elif mode == "invert":
            r, g, b = 1 - r, 1 - g, 1 - b
        elif mode == "tint":
            r, g, b = r * tint[0], g * tint[1], b * tint[2]
        else:
            raise ValueError(f"unknown mode {mode}")
        return pack555(r, g, b)
    return xf


def recolor_palette(mpr: bytes, transform) -> bytes:
    """Apply a per-entry u16->u16 transform to the palette inside mpr19116.3."""
    vals = read_palette(mpr)
    vals = [transform(v) for v in vals]
    return write_palette(mpr, vals)


def build_romset(src_zip: Path, dst_zip: Path, transform):
    """Copy every file of src_zip into dst_zip, patching only MPR_FILE."""
    changed = None
    with zipfile.ZipFile(src_zip, "r") as zin:
        names = zin.namelist()
        with zipfile.ZipFile(dst_zip, "w", zipfile.ZIP_DEFLATED) as zout:
            for n in names:
                data = zin.read(n)
                if n == MPR_FILE:
                    new = recolor_palette(data, transform)
                    assert len(new) == len(data), "mpr size changed!"
                    changed = (len(data), new)
                    data = new
                zout.writestr(n, data)
    if changed is None:
        raise SystemExit(f"{MPR_FILE} not found in {src_zip}")
    return changed


def main():
    ap = argparse.ArgumentParser(description="Die Hard Arcade (ST-V) recolor mod")
    ap.add_argument("--in",  dest="src", required=True, help="original diehard romset .zip")
    ap.add_argument("--out", dest="dst", required=True, help="output modified .zip")
    ap.add_argument("--mode", default="hue", choices=["hue", "swap_rb", "invert", "tint"])
    ap.add_argument("--hue", type=float, default=0.5, help="hue rotation 0..1 (mode=hue)")
    ap.add_argument("--tint", default="1,1,1", help="r,g,b multipliers (mode=tint)")
    a = ap.parse_args()
    tint = tuple(float(x) for x in a.tint.split(","))
    xf = make_transform(a.mode, hue=a.hue, tint=tint)
    n, _ = build_romset(Path(a.src), Path(a.dst), xf)
    print(f"patched {MPR_FILE} ({n} bytes, size unchanged) -> {a.dst}")
    print(f"palette: {NENT} entries at mpr offset 0x{PAL_IN_MPR:x} "
          f"(cart 0x{PAL_CART:x} / SH-2 0x{0x2000000+PAL_CART:08x})")


if __name__ == "__main__":
    main()
