#!/usr/bin/env python3
"""SH-2 function extents / compilation-unit splitter for Die Hard Arcade (ST-V).

Adapted from masonc15/tgm2p-decomp tools/fn.py (same compiler + chip).
Addresses are offsets into cart.bin (SH2 addr = offset + 0x02000000).
The fpr program region is ~0x200000..0x290000; real code starts ~0x200400.

usage: fn.py 0x214a06 [...]     annotated listing
       fn.py --units            one line per compilation unit (pool group)
       fn.py --scan             one line per call target: addr len shape
"""
import argparse
import struct
from pathlib import Path

import capstone as cs

ROM = (Path.home()/"decomp/diehard-stv/cart.bin").read_bytes()
CODE_START = 0x200400
TEXT_END = 0x290000
MD = cs.Cs(cs.CS_ARCH_SH, cs.CS_MODE_SH2 | cs.CS_MODE_BIG_ENDIAN)
MD.skipdata = True


def hw(a: int) -> int:
    return ROM[a] << 8 | ROM[a + 1]


def sdisp(v: int, bits: int) -> int:
    return v - (1 << bits) if v & (1 << (bits - 1)) else v


def extent(start: int, limit: int = 0x2000):
    """Return (code_end, extent_end, pool_refs, branch_targets)."""
    pc = start
    far = start
    pool = set()
    targets = set()
    resume = None
    after_jump = False
    while pc < start + limit:
        if pc == resume:
            after_jump = True
        if pc in pool or (after_jump and (pc in all_pool() or (hw(pc) in (0x0009, 0xFFFF, 0x0000)
                                                               and pc + 2 in all_pool()))):
            pc += 2
            continue
        after_jump = False
        w = hw(pc)
        op = w >> 12
        if op == 0xD:  # mov.l @(disp,pc)
            a = ((pc + 4) & ~3) + (w & 0xFF) * 4
            pool.update((a, a + 2))
        elif op == 0x9:  # mov.w @(disp,pc)
            pool.add(pc + 4 + (w & 0xFF) * 2)
        elif (w & 0xFF00) == 0xC700:  # mova
            pool.add(((pc + 4) & ~3) + (w & 0xFF) * 4)
        elif op == 0x8 and (w >> 8) & 0xF in (0x9, 0xB, 0xD, 0xF):  # bt/bf(/s)
            t = pc + 4 + sdisp(w & 0xFF, 8) * 2
            targets.add(t)
            far = max(far, t)
        elif op == 0xA:  # bra
            t = pc + 4 + sdisp(w & 0xFFF, 12) * 2
            targets.add(t)
            far = max(far, t)
            if pc + 2 >= far:
                pc += 4
                break
            resume = pc + 4
        elif w == 0x000B or (op == 0x4 and (w & 0xFF) == 0x2B):  # rts / jmp @Rn
            if pc + 2 >= far:
                pc += 4
                break
            resume = pc + 4
        pc += 2
    code_end = pc
    end = code_end
    beyond = [a for a in pool if a >= code_end]
    if beyond:
        end = max(beyond) + 2
    if end % 4 and hw(end) == 0x0009:
        end += 2
    return code_end, end, pool, targets


_ALL_POOL = None


def all_pool():
    global _ALL_POOL
    if _ALL_POOL is None:
        out = set()
        for pc in range(CODE_START, TEXT_END, 2):
            w = hw(pc)
            if w >> 12 == 0xD or w & 0xFF00 == 0xC700:
                a = ((pc + 4) & ~3) + (w & 0xFF) * 4
                out.update((a, a + 2))
            elif w >> 12 == 0x9:
                out.add(pc + 4 + (w & 0xFF) * 2)
        _ALL_POOL = out
    return _ALL_POOL


def shape(start: int, code_end: int) -> str:
    ins = list(MD.disasm(ROM[start:code_end], start))
    txt = [f"{i.mnemonic} {i.op_str}" for i in ins[:10]]
    pushes = sum(1 for t in txt if t.startswith("mov.l r") and "@-r15" in t)
    spills = sum(1 for t in txt if t.startswith(("mov.b r4", "mov.w r4", "mov.l r4,@(", "mov.l r4,@r15"))
                 or t.startswith(("mov.b r5", "mov.w r5", "mov.l r5,@")))
    if pushes == 0 and spills and any("r15" in t for t in txt[:4]):
        return "O0"
    return "opt"


def listing(start: int) -> None:
    code_end, end, pool, targets = extent(start)
    print(f"--- {start:#x}  code {code_end - start:#x}  extent {end - start:#x}  shape {shape(start, code_end)}")
    for i in MD.disasm(ROM[start:code_end], start):
        extra = ""
        if i.mnemonic in ("mov.l", "mov.w") and i.op_str.startswith("0x"):
            t = int(i.op_str.split(",")[0], 16)
            v = struct.unpack(">I", ROM[t:t + 4])[0] if i.mnemonic == "mov.l" else struct.unpack(">h", ROM[t:t + 2])[0]
            extra = f"   ; ={v:#x}"
        mark = ">" if i.address in targets else " "
        print(f"{mark}{i.address:06x} {i.bytes.hex():4s}  {i.mnemonic:7s} {i.op_str}{extra}")
    for a in range(code_end, end, 2):
        print(f" {a:06x} {hw(a):04x}  .word")


def units():
    """Runs of functions sharing one literal pool = translation units."""
    groups = []
    pc = unit_start = CODE_START
    members = []
    refs = set()
    while pc < TEXT_END:
        code_end, _, r, _ = extent(pc)
        members.append(pc)
        refs |= r
        q = code_end
        pad = (0x0000, 0x0009)
        if q in refs or (hw(q) in pad and q + 2 in refs):
            while q < TEXT_END and (q in refs or (hw(q) in pad and q + 2 in refs)):
                q += 2
            groups.append((unit_start, code_end, q, members))
            members, refs, unit_start = [], set(), q
        elif hw(q) == 0x0009 and q % 4:
            q += 2
        pc = q
    if members:
        groups.append((unit_start, pc, pc, members))
    return groups


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("addrs", nargs="*")
    ap.add_argument("--units", action="store_true")
    args = ap.parse_args()
    if args.units:
        groups = units()
        print(f"# {len(groups)} compilation units in {CODE_START:#x}..{TEXT_END:#x}")
        for start, pstart, end, members in groups:
            shapes = "".join("0" if shape(m, extent(m)[0]) == "O0" else "o" for m in members)
            gap = "" if members[0] == start else f" (starts {members[0]:x})"
            print(f"{start:06x}-{end:06x} len {end - start:5x} pool {end - pstart:4x} funcs {len(members):2d} {shapes}{gap}")
        return
    for a in args.addrs:
        listing(int(a, 16))


if __name__ == "__main__":
    main()
