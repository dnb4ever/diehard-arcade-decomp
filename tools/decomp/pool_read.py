#!/usr/bin/env python3
"""Alignment-aware literal-pool reader for the Die Hard Arcade (ST-V) boot/init block.

Why this exists
---------------
In the 0x200xxx boot/init cluster several `mov.l @(disp,pc),Rn` literal pools decode to
32-bit values whose two 16-bit halves are in the opposite order from the real constant:
capstone/fn report e.g. 0xe5700602, but the value the running game actually uses is
0x0602e570 (a valid HWRAM address). The tell is unambiguous: the RAW longword's high
half is garbage (0xE5xx/0xCDxx/...), out of every valid address window, while the
HALFWORD-SWAPPED value lands squarely in HWRAM (0x0600_0000..0x060F_FFFF) or a hardware
register window. (The naive 4-byte pool read is word-order-ambiguous for these 2-aligned
boot pools; this tool disambiguates by validity so the C can name the right g_<addr>.)

This is READ-ONLY: it never edits fn.py / m2c_fn.py. It reuses fn.extent for the window.

A value is "valid" if it falls in one of the SH-2 address windows the game uses:
  code/ROM     0x02000000..0x02900000
  HWRAM        0x06000000..0x06100000
  LWRAM        0x00200000..0x00300000
  VDP1 VRAM/FB 0x25c00000..0x25e00000
  VDP2 VRAM    0x25e00000..0x25f00000  (+ regs 0x25f80000..0x25f90000)
  SCU/SMPC/CS  0x25a00000..0x25d00000, 0x20000000..0x20100000
  on-chip      0xffff0000..0x100000000

usage:
  pool_read.py 0x2005fa                # per-mov.l pool: raw / swapped / which is valid
  pool_read.py --swap-only 0x2005fa    # print only the recommended (valid) constants
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import fn  # noqa: E402

MASK = 0xFFFFFFFF
SH2 = 0x02000000

WINDOWS = [
    (0x00200000, 0x00300000, "LWRAM"),
    (0x02000000, 0x02900000, "ROM/code"),
    (0x06000000, 0x06100000, "HWRAM"),
    (0x20000000, 0x20100000, "SMPC/sys"),
    (0x25a00000, 0x25b80000, "SCSP"),
    (0x25c00000, 0x25e00000, "VDP1"),
    (0x25e00000, 0x25f00000, "VDP2 VRAM"),
    (0x25f80000, 0x25f90000, "VDP2 reg"),
    (0x25d00000, 0x25d80000, "VDP1 reg"),
    (0x25fe0000, 0x25ff0000, "SCU reg"),
    (0xfffe0000, 0x100000000, "on-chip"),
]


def classify(v: int) -> str:
    for lo, hi, name in WINDOWS:
        if lo <= v < hi:
            return name
    return ""


def swap16(v: int) -> int:
    return ((v & 0xFFFF) << 16) | ((v >> 16) & 0xFFFF)


def pools(start: int):
    """Yield (ins_addr, mnemonic, pool_addr, size, raw, swapped) for every pc-relative
    pool load in the function at `start`."""
    code_end, _, _, _ = fn.extent(start)
    for i in fn.MD.disasm(fn.ROM[start:code_end], start):
        if i.mnemonic in ("mov.l", "mova") and i.op_str.startswith("0x"):
            a = int(i.op_str.split(",")[0], 16)
            raw = struct.unpack(">I", fn.ROM[a:a + 4])[0]
            yield (i.address, i.mnemonic, a, 4, raw, swap16(raw))
        elif i.mnemonic == "mov.w" and i.op_str.startswith("0x"):
            a = int(i.op_str.split(",")[0], 16)
            raw = struct.unpack(">H", fn.ROM[a:a + 2])[0]
            yield (i.address, i.mnemonic, a, 2, raw, raw)


def recommend(size: int, raw: int, swapped: int):
    """Return (value, note). For mov.w there is no ambiguity. For mov.l pick the half-order
    that yields a valid address window; if only one is valid, that is the answer; if both
    or neither are valid, keep raw and flag it for manual review."""
    if size == 2:
        return raw, "halfword"
    rok, sok = classify(raw), classify(swapped)
    if sok and not rok:
        return swapped, f"SWAPPED (raw invalid; swapped -> {sok})"
    if rok and not sok:
        return raw, f"raw ok ({rok})"
    if rok and sok:
        return raw, f"AMBIGUOUS (raw={rok}, swapped={sok}) - verify"
    return raw, "NEITHER valid - likely not an address (bitmask/immediate)"


def is_entry(a: int) -> bool:
    """True if `a` disassembles to a real instruction (mirrors find_entry.is_real_entry)."""
    if not (fn.CODE_START <= a < fn.TEXT_END) or a + 4 > len(fn.ROM):
        return False
    if a in fn.all_pool():
        return False
    ins = list(fn.MD.disasm(fn.ROM[a:a + 4], a))
    return bool(ins) and ins[0].mnemonic != ".byte"


def _reg_pool(ins_list, idx, reg):
    """Scan back for the pool constant last loaded into `reg` (mov.l @(..,pc)); follow one
    mov rA,rB copy. Return the raw longword, or None."""
    for _ in range(2):
        hit = None
        for j in range(idx - 1, -1, -1):
            ins = ins_list[j]
            ops = ins.op_str.replace(" ", "").split(",")
            if not ops or ops[-1] != reg:
                continue
            src = ops[0]
            if ins.mnemonic == "mov.l" and src.startswith("0x"):
                return struct.unpack(">I", fn.ROM[int(src, 16):int(src, 16) + 4])[0]
            if ins.mnemonic == "mov" and src.startswith("r"):
                reg, idx = src, j
                hit = True
                break
            return None
        if not hit:
            return None
    return None


def resolve_branches(start: int):
    """Report swap-resolved bsrf/braf/bra computed targets for the boot block. The boot
    pools store branch/pointer deltas halfword-swapped too (raw 0xe4680000 == 0x0000e468),
    so the valid target is PC+4+swap16(delta)."""
    code_end, _, _, _ = fn.extent(start)
    ins_list = list(fn.MD.disasm(fn.ROM[start:code_end], start))
    out = []
    for i, ins in enumerate(ins_list):
        if ins.mnemonic in ("bsrf", "braf"):
            reg = ins.op_str.strip()
            raw = _reg_pool(ins_list, i, reg)
            if raw is None:
                out.append((ins.address, ins.mnemonic, None, None, None))
                continue
            traw = (ins.address + 4 + raw) & MASK
            tswp = (ins.address + 4 + swap16(raw)) & MASK
            out.append((ins.address, ins.mnemonic, raw, traw, tswp))
    return out


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("addrs", nargs="+")
    ap.add_argument("--swap-only", action="store_true",
                    help="print only the recommended constant per load")
    ap.add_argument("--branches", action="store_true",
                    help="also swap-resolve bsrf/braf computed targets")
    args = ap.parse_args()
    for a in args.addrs:
        start = int(a, 16)
        print(f"--- {start:#x} (SH2 {start + SH2:#010x})")
        if args.branches:
            for iaddr, mn, raw, traw, tswp in resolve_branches(start):
                if raw is None:
                    print(f"    {iaddr:06x} {mn:5s} -> (delta not a single pool constant)")
                    continue
                pick = tswp if is_entry(tswp) and not is_entry(traw) else traw
                tag = ("SWAPPED -> real entry" if pick == tswp and pick != traw
                       else "raw" if is_entry(traw) else "neither is an entry")
                print(f"    {iaddr:06x} {mn:5s} raw->{traw:08x}[{'E' if is_entry(traw) else '-'}] "
                      f"swap->{tswp:08x}[{'E' if is_entry(tswp) else '-'}]  "
                      f"=> {pick:08x} (SH2 {pick + SH2:#x})  {tag}")
        for iaddr, mn, pa, size, raw, swp in pools(start):
            val, note = recommend(size, raw, swp)
            if args.swap_only:
                print(f"    {iaddr:06x} {mn:5s} {pa:06x} = {val:#010x}  "
                      f"(SH2 {val + (SH2 if val < 0x02000000 else 0):#x} "
                      f"[{classify(val) or '?'}])  {note}")
            else:
                print(f"    {iaddr:06x} {mn:5s} pool@{pa:06x}  raw={raw:#010x}"
                      f"[{classify(raw) or '?'}]  swapped={swp:#010x}"
                      f"[{classify(swp) or '?'}]  -> {val:#010x}  {note}")


if __name__ == "__main__":
    main()
