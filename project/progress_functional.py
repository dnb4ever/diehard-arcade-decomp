#!/usr/bin/env python3
"""Functional-coverage tracker for the Die Hard Arcade (ST-V) decomp.

Separate from progress.py (which reports ONLY the byte-exact oracle). This counts
how many of the ROM's functions have *any* C written for them, split by fidelity:

  functional  : project/src_functional/*.c  (equivalence-oriented C)
  nonmatching : project/src_nonmatching/*.c  (m2c drafts / not yet clean)
  byte-exact  : project/src/*.c              (byte-matching source)

The denominator is every function fn.py's splitter finds in the code region
(fn.units -> members), the same universe progress.py uses.

CANONICAL FILENAME CONVENTION (batch 7)
---------------------------------------
One .c file == one ROM function, named by its address. Two name forms exist in
the tree and MUST compare equal:
  * SH2-address form  : 022xxxxx.c   (cart offset + 0x02000000)  <-- canonical
  * cart-offset form  :  2xxxxx.c    (raw cart.bin offset)
Leading zeros are cosmetic (int(stem,16) ignores them). A file is mapped to its
member by *membership test*, not a ">=0x02000000" heuristic (that heuristic
mis-handles low SH2 addresses like the boot cluster, e.g. SH2 0x0201b8e whose
int value collides with in-universe member cart 0x201b8e). For each file:
  v = int(stem,16);  if v is a member -> cart form;  elif v-0x02000000 is a
  member -> SH2 form;  else the file is OUT-OF-UNIVERSE (boot cluster below
  code_start, or a find_entry-corrected / decomp_bounded TU entry that fn.units
  does not list as a member) -- a real function, but not one of the denominator
  members, so it is counted separately as an "extra".

Counts are deduped by canonical address, so the same function written under both
name forms (or present in two dirs) is counted once.
"""
import glob
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "tools", "decomp"))
import fn

SH2 = 0x02000000


def member_set():
    seen = set()
    for (_s, _ps, _e, members) in fn.units():
        seen.update(members)
    return seen


def file_cart(stem, members):
    """(canonical_key, in_universe). key is the member cart offset when the file
    names a member (either form); otherwise a normalized out-of-universe key
    (both name forms collapse to one so extras dedup too)."""
    try:
        v = int(stem, 16)
    except ValueError:
        return None, False
    if v in members:
        return v, True
    if (v - SH2) in members:
        return v - SH2, True
    return (v - SH2 if v >= SH2 else v), False


def dir_keys(subdir, members):
    d = os.path.join(HERE, subdir)
    inu, oob = set(), set()
    for p in glob.glob(os.path.join(d, "*.c")):
        stem = os.path.splitext(os.path.basename(p))[0]
        k, ok = file_cart(stem, members)
        if k is None:
            continue
        (inu if ok else oob).add(k)
    return inu, oob


def main():
    members = member_set()
    total = len(members)

    f_in, f_oob = dir_keys("src_functional", members)
    n_in, n_oob = dir_keys("src_nonmatching", members)
    s_in, s_oob = dir_keys("src", members)

    # byte-exact oracle verdicts
    oracle = 0
    cp = os.path.join(HERE, "build_results.json")
    if os.path.exists(cp):
        oracle = sum(1 for r in json.load(open(cp)).values() if r.get("exact"))

    any_in = f_in | n_in | s_in           # unique members with ANY C
    any_oob = f_oob | n_oob | s_oob       # unique out-of-universe functions
    pc = lambda n: 100.0 * n / total if total else 0.0

    print("=== Die Hard Arcade (ST-V) functional decomp coverage ===")
    print(f"  functions total (fn.units members) : {total}")
    print( "  (unique canonical functions; cart-offset & SH2-addr filename forms")
    print( "   normalized by membership test and deduped. 'extra' = real function")
    print( "   decompiled but not in the fn.units member list: boot cluster or a")
    print( "   find_entry/decomp_bounded corrected entry.)")
    print( "  --------------------------------------------------")
    print(f"  functional  (src_functional): {len(f_in):4d}   {pc(len(f_in)):6.2f}%   (+{len(f_oob)} extra)")
    print(f"  nonmatching (src_nonmatching): {len(n_in):4d}   {pc(len(n_in)):6.2f}%   (+{len(n_oob)} extra)")
    print(f"  byte-exact  (src files)      : {len(s_in):4d}   {pc(len(s_in)):6.2f}%   (+{len(s_oob)} extra)")
    print(f"  byte-exact  (oracle EXACT)   : {oracle:4d}   {pc(oracle):6.2f}%")
    print( "  --------------------------------------------------")
    print(f"  ANY C, member-covered (deduped) : {len(any_in):4d}   {pc(len(any_in)):6.2f}%")
    print(f"  ANY C, out-of-universe extras   : {len(any_oob):4d}")
    print(f"  ANY C functions total           : {len(any_in) + len(any_oob):4d}")


if __name__ == "__main__":
    main()
