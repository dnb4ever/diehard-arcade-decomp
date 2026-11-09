#!/usr/bin/env python3
"""Honest function-count census for the Die Hard Arcade (ST-V) decomp.

PURPOSE
-------
progress_functional.py reports coverage against a denominator of 1272
"fn.units members" -- every function-start fn.py's forced-disassembly sweep
finds in 0x200400..0x290000. A large chunk of the upper region
(~0x24c000..0x2896xx) is DATA the sweep carved into pseudo-functions, so those
members are phantoms. This script computes an HONEST denominator (genuine SH-2
code only) using TWO INDEPENDENT signals and reports where they agree.

SIGNAL 1 -- byte-fraction (per member)
    Disassemble each member body [start, code_end) with capstone (SH2, big-endian,
    skipdata=on) and compute the fraction of slots that fail to decode
    (".byte"/invalid). Real SHC code decodes almost entirely; data does not.
    Calibrated thresholds (evidence in notes/honest_denominator.md):
        frac <= 0.10        -> code
        0.10 < frac < 0.35  -> ambiguous   (table-heavy real code OR short data)
        frac >= 0.35        -> data

SIGNAL 2 -- triage category (notes/function_triage.tsv)
    An independent call-graph / segmentation triage already labelled each member
    callC / leafC / asm (genuine, reachable code) or misseg (mis-segmented =
    data-as-code suspect).

HONEST DENOMINATOR = genuine-code members = callC + leafC + asm.
The two signals cross-validate: ~99% of that set is byte-fraction code/ambiguous,
and 417/418 hand-decompiled functions fall inside it.

CAVEATS (reported, not hidden)
    * byte-fraction is NECESSARY but not SUFFICIENT: short sequential-value data
      tables can disassemble as valid-looking SH2 (the confirmed-data family at
      0x267e92..0x26dcbe reads frac ~0.29-0.42), and trivial 4-byte members decode
      to 2 valid-looking slots (frac 0). Member SIZE and BAND matter; see the
      per-band table and the agreement matrix.
    * the 0.10-0.35 ambiguous zone genuinely overlaps code and data -> the
      denominator is given as a RANGE.

usage: census.py            full report
       census.py --tsv      per-member rows
"""
import csv
import glob
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "tools", "decomp"))
import fn
import capstone as cs

SH2 = 0x02000000
CODE_T = 0.10
DATA_T = 0.35
DATA_BAND = 0x24C000
TRIAGE = os.path.join(ROOT, "notes", "function_triage.tsv")

MD = cs.Cs(cs.CS_ARCH_SH, cs.CS_MODE_SH2 | cs.CS_MODE_BIG_ENDIAN)
MD.skipdata = True
ROM = fn.ROM


def members():
    seen = set()
    for (_s, _ps, _e, mem) in fn.units():
        seen.update(mem)
    return sorted(seen)


def byte_fraction(start, end):
    if end <= start:
        return None, 0
    total = bad = 0
    for i in MD.disasm(bytes(ROM[start:end]), start):
        total += 1
        if i.id == 0 or i.mnemonic in ("", ".byte"):
            bad += 1
    if total == 0:
        return None, 0
    return bad / total, total


def classify(frac):
    if frac is None:
        return "empty"
    if frac <= CODE_T:
        return "code"
    if frac >= DATA_T:
        return "data"
    return "ambiguous"


def load_triage(mset):
    """cart_addr -> category, from notes/function_triage.tsv (sh2_addr col)."""
    cat = {}
    if not os.path.exists(TRIAGE):
        return cat
    with open(TRIAGE) as fh:
        for row in csv.DictReader(fh, delimiter="\t"):
            try:
                a = int(row["sh2_addr"], 16) - SH2
            except (ValueError, KeyError):
                continue
            if a in mset:
                cat[a] = row.get("category", "")
    return cat


def functional_members(mset):
    out = set()
    d = os.path.join(ROOT, "project", "src_functional")
    for p in glob.glob(os.path.join(d, "*.c")):
        stem = os.path.splitext(os.path.basename(p))[0]
        try:
            v = int(stem, 16)
        except ValueError:
            continue
        if v in mset:
            out.add(v)
        elif (v - SH2) in mset:
            out.add(v - SH2)
    return out


BANDS = [
    (0x200400, 0x210000, "0x200400-0x210000 lower code"),
    (0x210000, 0x220000, "0x210000-0x220000 lower code"),
    (0x220000, 0x230000, "0x220000-0x230000 lower code"),
    (0x230000, 0x240000, "0x230000-0x240000 lower code"),
    (0x240000, 0x248000, "0x240000-0x248000 lower code"),
    (0x248000, 0x24C000, "0x248000-0x24c000 upper code pocket"),
    (0x24C000, 0x260000, "0x24c000-0x260000 upper (data)"),
    (0x260000, 0x270000, "0x260000-0x270000 upper (data)"),
    (0x270000, 0x280000, "0x270000-0x280000 upper (data)"),
    (0x280000, 0x290000, "0x280000-0x290000 upper (data)"),
]
GENUINE_CATS = ("callC", "leafC", "asm")


def band_of(a):
    for lo, hi, name in BANDS:
        if lo <= a < hi:
            return name
    return "other"


def build(ms, cat):
    rows = []
    for a in ms:
        code_end = fn.extent(a)[0]
        frac, n = byte_fraction(a, code_end)
        rows.append({
            "addr": a, "code_end": code_end, "frac": frac, "n": n,
            "cls": classify(frac), "band": band_of(a),
            "cat": cat.get(a, ""),
        })
    return rows


def main():
    ms = members()
    mset = set(ms)
    cat = load_triage(mset)
    rows = build(ms, cat)
    fmem = functional_members(mset)
    total = len(rows)

    if "--tsv" in sys.argv:
        print("sh2_addr\tcart_addr\tcode_end\tfrac\tninsn\tbytefrac_class\ttriage_cat\tband")
        for r in rows:
            fr = "" if r["frac"] is None else f"{r['frac']:.3f}"
            print(f"{r['addr']+SH2:08x}\t{r['addr']:06x}\t{r['code_end']:06x}\t{fr}\t"
                  f"{r['n']}\t{r['cls']}\t{r['cat']}\t{r['band']}")
        return

    def nby(c):
        return sum(1 for r in rows if r["cls"] == c)

    genuine = {r["addr"] for r in rows if r["cat"] in GENUINE_CATS}
    misseg = {r["addr"] for r in rows if r["cat"] == "misseg"}
    have_triage = bool(cat)

    print("=== Die Hard Arcade (ST-V) honest function-count census ===")
    print(f"reported denominator (fn.units members) : {total}")
    print()
    print("--- signal 1: byte-fraction disasm screen ---")
    print(f"  code (frac<={CODE_T})      : {nby('code'):4d}")
    print(f"  ambiguous ({CODE_T}-{DATA_T}) : {nby('ambiguous'):4d}")
    print(f"  data (frac>={DATA_T})      : {nby('data'):4d}   (phantom data-as-code)")
    print()

    if have_triage:
        from collections import Counter
        tc = Counter(r["cat"] or "(none)" for r in rows)
        print("--- signal 2: triage category (function_triage.tsv) ---")
        for k in ("callC", "leafC", "asm", "misseg", "(none)"):
            if tc.get(k):
                print(f"  {k:8s} : {tc[k]:4d}")
        print()
        print("--- agreement matrix (triage x byte-fraction) ---")
        print(f"  {'':10s}{'code':>6}{'amb':>6}{'data':>6}{'total':>7}")
        for k in ("callC", "leafC", "asm", "misseg"):
            sub = [r for r in rows if r["cat"] == k]
            if not sub:
                continue
            c = sum(1 for r in sub if r["cls"] == "code")
            a = sum(1 for r in sub if r["cls"] == "ambiguous")
            d = sum(1 for r in sub if r["cls"] == "data")
            print(f"  {k:10s}{c:6d}{a:6d}{d:6d}{len(sub):7d}")
        print()

    print("--- by address band  (code / ambiguous / data / total) ---")
    for lo, hi, name in BANDS:
        b = [r for r in rows if r["band"] == name]
        if not b:
            continue
        c = sum(1 for r in b if r["cls"] == "code")
        a = sum(1 for r in b if r["cls"] == "ambiguous")
        d = sum(1 for r in b if r["cls"] == "data")
        print(f"  {name:38s} {c:4d} / {a:4d} / {d:4d} / {len(b):4d}")
    print()

    print("--- HONEST DENOMINATOR (genuine SH-2 code) ---")
    if have_triage:
        rowmap = {r["addr"]: r for r in rows}
        # genuine-set members above the data band are confirmed data tables
        # (0x2896xx/0x228c1xx: decode as valid-looking table entries, frac~0) that
        # the triage mislabeled -> drop from the denominator.
        gen_up = {a for a in genuine if a >= DATA_BAND}
        gen_lo = genuine - gen_up
        g = len(gen_lo)                        # central honest denominator
        g_data = sum(1 for a in gen_lo if rowmap[a]["cls"] == "data")
        # plausibly-real misseg: lower region, decodes as code/amb, not a trivial
        # fragment -> real functions the triage's misseg heuristic over-flagged.
        plaus = [r for r in rows if r["cat"] == "misseg" and r["addr"] < DATA_BAND
                 and r["cls"] in ("code", "ambiguous") and r["n"] >= 8]
        lo = g - g_data                        # also drop lower genuine that look like data
        hi = g + len(plaus)                    # add plausibly-real over-flagged misseg
        genuine = gen_lo                        # used by coverage block below
        print(f"  callC+leafC+asm (triage genuine)     : {len(gen_up)+len(gen_lo)}")
        print(f"    minus {len(gen_up)} confirmed upper data tables (>=0x24c000)")
        print(f"  central honest denominator           : {g}")
        print(f"    byte-fraction confirms code/amb    : "
              f"{sum(1 for a in gen_lo if rowmap[a]['cls'] in ('code','ambiguous'))}/{g}")
        print(f"  range                                : {lo} .. {hi}")
        print(f"    low  = central minus byte-data       ({g_data} dropped)")
        print(f"    high = central + plausible misseg    (+{len(plaus)} lower/non-trivial code)")
        print(f"  phantom total (1272 - central)       : {total - g}")
    else:
        g = nby("code") + nby("ambiguous")
        lo, hi = nby("code"), g
        print(f"  NO triage file -- byte-fraction only.")
        print(f"  range (code .. code+ambiguous)       : {lo} .. {hi}")
    print()

    print("--- FUNCTIONAL COVERAGE ---")
    print(f"  functional members (src_functional)  : {len(fmem)}")
    print(f"  REPORTED  : {len(fmem)}/{total} = {100.0*len(fmem)/total:.2f}%")
    if have_triage:
        ing = len(fmem & genuine)
        print(f"  decompiled-in-genuine                : {ing}/{len(fmem)}")
        print(f"  HONEST    : {len(fmem)}/{g} = {100.0*len(fmem)/g:.2f}%  "
              f"(range {100.0*len(fmem)/hi:.1f}%..{100.0*len(fmem)/lo:.1f}%)")
    else:
        print(f"  HONEST    : {len(fmem)}/{g} = {100.0*len(fmem)/g:.2f}%")


if __name__ == "__main__":
    main()
