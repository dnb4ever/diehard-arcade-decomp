#!/usr/bin/env python3
"""Find the REAL entry address of a Die Hard Arcade (ST-V) function.

Why this exists
---------------
fn.units() groups functions by shared literal pool. At a unit boundary the
previous function's literal pool occasionally spills past the point where the
grouper stops absorbing pool words, so the NEXT unit's reported first member
lands a few words INSIDE that trailing pool. Disassembling from there yields
garbage (capstone emits `.byte` skipdata) and m2c then prints bogus constants
and field refs -- this is the "0x4F22 / -0x2D3C / entity-base-lost" symptom
(mis-attributed to GBR in an earlier pass; there is no GBR entity base -- the
entity is the r5 argument, moved to r14).

A real SHC function entry always disassembles to a genuine instruction (prologue
or leaf setup). A pool-contaminated address disassembles as `.byte` skipdata
and/or sits inside fn.all_pool(). When a unit's first member is contaminated,
the real entry is the PREVIOUS function's extent-end (extent()[1]), i.e. the
first address after that function's code AND its own trailing pool.

This never edits fn.py / m2c_fn.py; it is a read-only helper.

usage:
  find_entry.py 0x2296da          # -> corrected real entry for one address
  find_entry.py --band 0x228100 0x22a400   # list corrected entries in a range
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import fn  # noqa: E402

ROM = fn.ROM


def is_real_entry(addr: int) -> bool:
    """True if `addr` disassembles to a real instruction (not pool/skipdata)."""
    if addr in fn.all_pool():
        return False
    ins = list(fn.MD.disasm(ROM[addr:addr + 4], addr))
    return bool(ins) and ins[0].mnemonic != ".byte"


def corrected_members():
    """Yield (unit, [real member entries]) with contaminated first members fixed."""
    prev_end = None
    for start, pstart, end, mem in fn.units():
        members = list(mem)
        if members and not is_real_entry(members[0]) and prev_end is not None:
            members[0] = prev_end
        yield (start, end, members)
        prev_end = fn.extent(members[-1])[1]


def fix(addr: int) -> int:
    """Return the real entry for `addr` (itself if already real)."""
    if is_real_entry(addr):
        return addr
    # find the unit containing addr; return its corrected first member if that
    # first member <= addr < its code-end, else scan forward to next real entry.
    for start, end, members in corrected_members():
        if start <= addr < end:
            for m in members:
                if m >= addr and is_real_entry(m):
                    return m
            if members and is_real_entry(members[0]):
                return members[0]
    a = addr
    while a < fn.TEXT_END and not is_real_entry(a):
        a += 2
    return a


def main() -> None:
    args = sys.argv[1:]
    if args and args[0] == "--band":
        lo, hi = int(args[1], 16), int(args[2], 16)
        for start, end, members in corrected_members():
            if not (lo <= start <= hi):
                continue
            print(f"=== unit {start:06x}-{end:06x} ===")
            for m in members:
                ce, _, _, _ = fn.extent(m)
                ok = is_real_entry(m)
                print(f"  {m+0x02000000:08x} cart={m:06x} code={ce-m:#5x} "
                      f"first={ROM[m]<<8|ROM[m+1]:04x} real={ok}")
        return
    for a in args:
        addr = int(a, 16)
        r = fix(addr)
        tag = "(already real)" if r == addr else f"(was {addr:06x}, pool-contaminated)"
        print(f"{addr:#08x} -> real entry {r+0x02000000:#010x} (cart {r:06x}) {tag}")


if __name__ == "__main__":
    main()
