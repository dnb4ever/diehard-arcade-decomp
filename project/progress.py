#!/usr/bin/env python3
"""Progress tracker for the Die Hard Arcade (ST-V) decomp.

Denominator = every function the splitter (fn.units) finds in the code region,
summed as code+pool extent bytes (the unit match.py/build.py verify).
Numerator   = extent bytes of functions proven EXACT by build.py
              (read from build_results.json).

usage:
  progress.py            report % matched (uses cached build_results.json)
  progress.py --build    run build.py first, then report
  progress.py --code     count code-only bytes (exclude literal pools)
"""
import os
import sys
import json
import subprocess
import tomllib

HERE = os.path.dirname(os.path.abspath(__file__))
HOME = os.path.expanduser("~")
sys.path.insert(0, os.path.join(HOME, "decomp/diehard-stv/tools/decomp"))
import fn


def all_functions():
    seen = set()
    for (_s, _ps, _e, members) in fn.units():
        for m in members:
            seen.add(m)
    out = {}
    for m in sorted(seen):
        ce, en, _p, _t = fn.extent(m)
        out[m] = {"code": ce - m, "extent": en - m}
    return out


def main():
    use_code = "--code" in sys.argv
    if "--build" in sys.argv:
        subprocess.run([sys.executable, os.path.join(HERE, "build.py")])

    funcs = all_functions()
    key = "code" if use_code else "extent"
    total_bytes = sum(v[key] for v in funcs.values())
    total_funcs = len(funcs)

    cfg = tomllib.load(open(os.path.join(HERE, "splits.toml"), "rb"))
    reserved = set()
    for r in cfg.get("reserved", []):
        reserved.update(r.get("addrs", []))
    declared = {f["addr"] for f in cfg.get("func", []) if f.get("status") == "matched"}

    cache = {}
    cp = os.path.join(HERE, "build_results.json")
    if os.path.exists(cp):
        cache = json.load(open(cp))

    matched_funcs = []
    matched_bytes = 0
    for hexaddr, r in cache.items():
        if r.get("exact"):
            a = int(hexaddr, 16)
            b = funcs.get(a, {}).get(key, r.get("extent", 0))
            matched_bytes += b
            matched_funcs.append(a)

    unverified = sorted(declared - set(matched_funcs))
    pct = 100.0 * matched_bytes / total_bytes if total_bytes else 0.0

    print(f"=== Die Hard Arcade (ST-V) decomp progress [{key} bytes] ===")
    print(f"  functions total     : {total_funcs}")
    print(f"  functions matched   : {len(matched_funcs)}")
    print(f"  functions reserved  : {len(reserved)} (other agents)")
    print(f"  bytes total         : {total_bytes} (0x{total_bytes:x})")
    print(f"  bytes matched       : {matched_bytes} (0x{matched_bytes:x})")
    print(f"  ---------------------------------------")
    print(f"  MATCHED             : {pct:.4f}%")
    if unverified:
        print(f"  NOTE: {len(unverified)} declared-matched not EXACT in cache "
              f"(run --build): {[hex(a) for a in unverified]}")
    if not cache:
        print("  (no build_results.json yet - run: python progress.py --build)")


if __name__ == "__main__":
    main()
