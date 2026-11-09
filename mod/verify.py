#!/usr/bin/env python3
"""verify.py -- confirm the recolor patch WITHOUT an emulator.

Re-runs build_cart.py's exact transform (word_swap + placement) on the MODIFIED
romset, rebuilds the palette region of cart.bin in memory, and checks:
  1. cart 0x9bb438 now holds the NEW colours (hue-shifted),
  2. the reconstructed cart differs from the original ONLY inside the palette,
  3. every other romset file is byte-identical (CRC/size) to the original,
  4. mpr19116.3 keeps its exact size (MAME audits size + CRC).
Does NOT touch the real cart.bin.
"""
import struct, zipfile, zlib
from pathlib import Path

ROOT = (Path.home()/"decomp/diehard-stv")
ORIG_ZIP = ROOT / "diehard.zip"
MOD_ZIP  = ROOT / "mod" / "diehard_recolor.zip"
CARTBIN  = ROOT / "cart.bin"            # original reference, read-only
MPR_PLACE = 0x800000
PAL_CART  = 0x9bb438
NENT = 0x18ac
PAL_BYTES = NENT * 2

# ---- build_cart.py's exact word_swap ----
def word_swap(b):
    b = bytearray(b); b[0::2], b[1::2] = b[1::2], b[0::2]; return bytes(b)

def exp5(c): return (c << 3) | (c >> 2)
def decode(v):
    v &= 0x7fff
    return [exp5(v & 31), exp5((v >> 5) & 31), exp5((v >> 10) & 31)]

def zread(z, n):
    with zipfile.ZipFile(z) as zf: return zf.read(n)

def crc_map(z):
    with zipfile.ZipFile(z) as zf:
        return {i.filename: (i.CRC, i.file_size) for i in zf.infolist()}

orig_mpr = zread(ORIG_ZIP, "mpr19116.3")
mod_mpr  = zread(MOD_ZIP,  "mpr19116.3")

# --- build_cart.py placement: cart[0x800000:] = word_swap(mpr19116.3) ---
orig_cart_region = word_swap(orig_mpr)[PAL_CART-MPR_PLACE : PAL_CART-MPR_PLACE+PAL_BYTES]
mod_cart_region  = word_swap(mod_mpr )[PAL_CART-MPR_PLACE : PAL_CART-MPR_PLACE+PAL_BYTES]

# Cross-check orig against the real cart.bin that build_cart.py produced earlier
real = CARTBIN.read_bytes()[PAL_CART:PAL_CART+PAL_BYTES]
print("1) orig reconstruction matches existing cart.bin palette :", orig_cart_region == real)

orig_vals = struct.unpack(f">{NENT}H", orig_cart_region)
mod_vals  = struct.unpack(f">{NENT}H", mod_cart_region)
ndiff = sum(1 for a,b in zip(orig_vals, mod_vals) if a != b)
print(f"2) palette entries changed by patch                      : {ndiff}/{NENT}")
print("   sample (cidx -> old RGB  =>  new RGB):")
for c in [0, 10, 23, 100, 500, 6315]:
    print(f"     {c:5d}: {decode(orig_vals[c])} => {decode(mod_vals[c])}")

# 3) reconstructed MODIFIED cart == original everywhere EXCEPT palette region?
#    (only mpr19116.3 differs between zips; placement is byte-for-byte identical
#     outside the spliced region, so compare the two mprs directly)
diffs = [i for i in range(len(orig_mpr)) if orig_mpr[i] != mod_mpr[i]]
lo, hi = PAL_CART-MPR_PLACE, PAL_CART-MPR_PLACE+PAL_BYTES
outside = [i for i in diffs if not (lo <= i < hi)]
print(f"3) mpr19116.3 byte diffs total={len(diffs)}  outside palette={len(outside)}")
print(f"   diff byte range: 0x{min(diffs):x}..0x{max(diffs):x}  (palette win 0x{lo:x}..0x{hi:x})")

# 4) size + CRC of every file
oc, mc = crc_map(ORIG_ZIP), crc_map(MOD_ZIP)
print("4) per-file size/CRC audit (vs original):")
same_size_all = True
for name in sorted(oc):
    ocrc, osz = oc[name]; mcrc, msz = mc.get(name, (None,None))
    tag = "unchanged" if (ocrc==mcrc and osz==msz) else "CHANGED"
    if name == "mpr19116.3":
        same = "size OK, CRC differs (expected)" if (osz==msz and ocrc!=mcrc) else "PROBLEM"
        print(f"   {name:16s} size {osz}->{msz} CRC {ocrc:08x}->{mcrc:08x}  [{same}]")
        if osz != msz: same_size_all = False
    elif tag == "CHANGED":
        print(f"   {name:16s} [{tag}] size {osz}->{msz} CRC {ocrc:08x}->{mcrc:08x}")
print("   all other files byte-identical:",
      all(oc[n]==mc.get(n) for n in oc if n!="mpr19116.3"))
print("   mpr19116.3 size preserved      :", oc["mpr19116.3"][1]==mc["mpr19116.3"][1])
print("   mpr19116.3 orig/new CRC32      : %08x / %08x" %
      (oc["mpr19116.3"][0], mc["mpr19116.3"][0]))
