#!/usr/bin/env python3
"""Reconstruct the Die Hard Arcade (ST-V, set 'diehard') cartridge image
exactly as the big-endian SH-2 addresses it, per MAME src/mame/sega/stv.cpp.

Region 'cart' maps to SH-2 0x02000000, ROM_REGION32_BE, ERASEFF.
  fpr19119.13  ROM_LOAD16_BYTE   @0x0000001 len 0x100000  (odd-byte interleave)
               ROM_RELOAD_PLAIN  @0x0200000 len 0x100000  (contiguous, usable)
               ROM_RELOAD_PLAIN  @0x0300000 len 0x100000
  mpr19115.2   ROM_LOAD16_WORD_SWAP @0x0400000 len 0x400000
  mpr19116.3   ROM_LOAD16_WORD_SWAP @0x0800000 len 0x400000
  mpr19117.4   ROM_LOAD16_WORD_SWAP @0x0c00000 len 0x400000
  mpr19118.5   ROM_LOAD16_WORD_SWAP @0x1000000 len 0x400000
"""
import os, string

ROMDIR = os.path.join(os.path.dirname(__file__), "diehard_rom")
OUT    = os.path.join(os.path.dirname(__file__), "cart.bin")
SIZE   = 0x1400000  # cover through end of mpr19118

def load(name):
    with open(os.path.join(ROMDIR, name), "rb") as f:
        return f.read()

def word_swap(b):
    b = bytearray(b)
    b[0::2], b[1::2] = b[1::2], b[0::2]  # swap each 16-bit word's bytes
    return bytes(b)

cart = bytearray(b"\xff" * SIZE)

def place(data, off):
    cart[off:off+len(data)] = data

def place_byte_interleave(data, off):   # ROM_LOAD16_BYTE: byte i -> off + 2*i
    for i, by in enumerate(data):
        cart[off + 2*i] = by

fpr = load("fpr19119.13")
place_byte_interleave(fpr, 0x0000001)   # the odd-byte copy MAME builds
place(fpr, 0x0200000)                    # contiguous (RELOAD_PLAIN) -- usable
place(fpr, 0x0300000)
place(word_swap(load("mpr19115.2")), 0x0400000)
place(word_swap(load("mpr19116.3")), 0x0800000)
place(word_swap(load("mpr19117.4")), 0x0c00000)
place(word_swap(load("mpr19118.5")), 0x1000000)

with open(OUT, "wb") as f:
    f.write(cart)
print(f"wrote {OUT} ({len(cart):#x} bytes, maps at SH-2 0x02000000)")

# ---- quick look: non-0xFF spans so we see where real content lives ----
print("\n== content map (non-0xFF regions, 64KB granularity) ==")
step = 0x10000
run_start = None
for a in range(0, SIZE, step):
    chunk = cart[a:a+step]
    has = any(c != 0xFF for c in chunk)
    if has and run_start is None:
        run_start = a
    elif not has and run_start is not None:
        print(f"  0x{run_start:07x} - 0x{a:07x}  (SH2 0x{0x2000000+run_start:08x})")
        run_start = None
if run_start is not None:
    print(f"  0x{run_start:07x} - 0x{SIZE:07x}  (SH2 0x{0x2000000+run_start:08x})")

# ---- hexdump helper ----
def hexdump(off, n=64, label=""):
    print(f"\n== hex @0x{off:07x} (SH2 0x{0x2000000+off:08x}) {label} ==")
    for r in range(0, n, 16):
        row = cart[off+r:off+r+16]
        hexs = " ".join(f"{c:02x}" for c in row)
        asc  = "".join(chr(c) if 32 <= c < 127 else "." for c in row)
        print(f"  {off+r:07x}: {hexs:<47}  {asc}")

for off, lbl in [(0x000000,"region start"), (0x200000,"fpr contiguous (boot?)"),
                 (0x400000,"mpr19115 start"), (0x1000000,"mpr19118 start")]:
    hexdump(off, 64, lbl)

# ---- fingerprint hunt: compiler / SGL / sega / version / dates ----
print("\n== fingerprint strings (compiler / SGL / SEGA / version) ==")
printable = set(bytes(string.printable, "ascii")) - set(b"\t\n\r\x0b\x0c")
def find_strings(minlen=5):
    cur = bytearray(); start = 0
    for i, c in enumerate(cart):
        if c in printable:
            if not cur: start = i
            cur.append(c)
        else:
            if len(cur) >= minlen:
                yield start, cur.decode("ascii", "replace")
            cur = bytearray()
    if len(cur) >= minlen:
        yield start, cur.decode("ascii", "replace")

import re
needles = re.compile(r"(SGL|SEGA|Hitachi|HITACHI|compiler|Compiler|COMPILER|"
                     r"[Vv]er\.?\s|version|Version|VERSION|AM1|SH-?2|SH7|"
                     r"Copyright|COPYRIGHT|\(C\)|19\d\d|GCC|gcc|SNASM|shc|"
                     r"TITAN|Titan|CART|BOOT)")
hits = 0
for off, s in find_strings(5):
    if needles.search(s):
        print(f"  0x{off:07x} (SH2 0x{0x2000000+off:08x}): {s[:80]!r}")
        hits += 1
        if hits > 60:
            print("  ... (truncated)")
            break
if not hits:
    print("  (no fingerprint strings found -- try the byte-swapped variant)")
