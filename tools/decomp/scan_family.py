#!/usr/bin/env python3
"""Scan all functions: classify by resolvable braf/bsrf; find tractable ones.

For each function unit, resolve its computed branches. Report:
 - functions that contain >=1 braf/bsrf
 - whether ALL are resolved (fully tractable by the wrapper) vs any table/unresolved
 - the set of resolved targets (so we can spot the 0x22dc2c animation_update family)

DONE-CHECK (batch 7): a function is "done" if src_functional/ has a .c for it,
matched by MEMBERSHIP (not a >=0x02000000 heuristic). Filenames exist in two forms
-- SH2-address 022xxxxx.c (canonical) and cart-offset 2xxxxx.c -- which must
compare equal. For a stem value v: v is a member -> cart form; v-0x02000000 is a
member -> SH2 form; else out-of-universe (not one of these members). This fixes
the batch-5/6 under-report where SH2-named files read as "not done".
"""
import os
import sys
from pathlib import Path

import braf_resolve
import fn

ANIM = 0x22dc2c  # animation_update cart offset
SH2 = 0x02000000

groups = fn.units()
members = []
member_set = set()
for start, pstart, end, mem in groups:
    members.extend(mem)
    member_set.update(mem)


def file_cart(stem):
    """Map a filename stem to its member cart offset (either name form), or None
    if the file is not one of these members (out-of-universe)."""
    try:
        v = int(stem, 16)
    except ValueError:
        return None
    if v in member_set:
        return v
    if (v - SH2) in member_set:
        return v - SH2
    return None


done_dir = (Path.home()/"decomp/diehard-stv/project/src_functional")
done = set()
for p in done_dir.glob("*.c"):
    c = file_cart(p.stem)
    if c is not None:
        done.add(c)

rows = []
for start in members:
    try:
        res = braf_resolve.resolve(start)
    except Exception as e:  # noqa: BLE001
        rows.append((start, "ERR", str(e), [], False))
        continue
    if not res:
        continue
    resolved = [e for e in res if e["status"] == "resolved"]
    bad = [e for e in res if e["status"] != "resolved"]
    targets = sorted({e["target"] for e in resolved})
    fully = len(bad) == 0 and len(resolved) > 0
    rows.append((start, "ok", res, targets, fully))

sh2 = lambda c: c + SH2
isdone = lambda s: s in done

print("=== functions whose braf/bsrf ALL resolve (wrapper-tractable) ===")
tract = [r for r in rows if r[1] == "ok" and r[4]]
for start, _, res, targets, _ in sorted(tract):
    tgt = " ".join(f"{t:06x}" for t in targets)
    anim = " <ANIM>" if ANIM in targets else ""
    mark = " [DONE]" if isdone(start) else ""
    code_end, _, _, _ = fn.extent(start)
    print(f"{sh2(start):08x} size {code_end-start:#5x} n={len(res)} -> {tgt}{anim}{mark}")

print(f"\ntotal fully-tractable: {len(tract)}  "
      f"(not yet done: {sum(1 for r in tract if not isdone(r[0]))})")

print("\n=== fully-tractable targeting animation_update 0x22dc2c, not done ===")
fam = [r for r in tract if ANIM in r[3] and not isdone(r[0])]
for start, _, res, targets, _ in sorted(fam):
    code_end, _, _, _ = fn.extent(start)
    print(f"{sh2(start):08x} size {code_end-start:#5x} targets {' '.join(f'{t:06x}' for t in targets)}")
print(f"anim-family tractable & not done: {len(fam)}")

print("\n=== partially-resisted (some table/unresolved) not done ===")
part = [r for r in rows if r[1] == "ok" and not r[4] and not isdone(r[0])]
for start, _, res, targets, _ in sorted(part):
    bad = [e for e in res if e["status"] != "resolved"]
    kinds = ",".join(sorted({e["status"] for e in bad}))
    print(f"{sh2(start):08x} n={len(res)} bad={len(bad)}({kinds}) resolved_tgts={' '.join(f'{t:06x}' for t in targets)}")
print(f"partially-resisted not done: {len(part)}")
