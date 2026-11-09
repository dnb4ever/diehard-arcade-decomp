#!/usr/bin/env python3
"""Batch per-function match verifier for the Die Hard Arcade (ST-V) decomp.

For every func in splits.toml with status="matched", compile its C with the
real Hitachi SHC v5.0r32 (under wibo), convert rof->elf, extract .text, and
byte-compare to the ROM function extent (code+pool) at its cart offset
(SH2 addr = offset + 0x02000000). This is the authoritative MATCH oracle.

Writes build_results.json (consumed by progress.py). Exits non-zero if any
function declared "matched" fails to byte-equal the ROM -> usable in CI.

usage:
  build.py                 verify every matched func
  build.py 0x223780 ...    verify only these addrs
  build.py --diff          on a mismatch, print the target|got instruction diff
  build.py --jobs N        parallel compiles (default 4)
"""
import os
import sys
import json
import argparse
import tomllib
import subprocess
import tempfile
import shutil
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
HOME = os.path.expanduser("~")
sys.path.insert(0, os.path.join(HOME, "decomp/diehard-stv/tools/decomp"))
import fn
import capstone as cs

COMP = os.path.join(HOME, "decomp/diehard-stv/tools/compilers")
SHC = os.path.join(COMP, "shc-v5.0r32")
CROSS = os.path.join(HOME, "decomp/diehard-stv/tools/cross/bin")
MD = cs.Cs(cs.CS_ARCH_SH, cs.CS_MODE_SH2 | cs.CS_MODE_BIG_ENDIAN)
MD.skipdata = True


def _dis(b):
    return [f"{i.mnemonic} {i.op_str}" for i in MD.disasm(b, 0)]


def compile_text(src_path, flags):
    """Compile one C file -> raw .text bytes (or None on compile failure)."""
    work = tempfile.mkdtemp(prefix="dhbuild_")
    try:
        subprocess.run(["bash", "-c", f"cp -r {SHC}/bin/. {work}/"], check=False)
        with open(f"{work}/src.c", "w", newline="\r\n") as f:
            f.write(open(src_path).read())
        r = subprocess.run(
            ["bash", "-c",
             f"cd {work} && SHC_LIB=. SHC_TMP=. {COMP}/wibo ./shc.exe src.c "
             f"-comment=nonest -cpu=sh2 -endian=big -sjis -string=const {flags} "
             f"-object=src.obj"],
            capture_output=True, text=True)
        if not os.path.exists(f"{work}/src.obj"):
            return None, (r.stdout or "")[-400:]
        subprocess.run(
            ["bash", "-c",
             f"python3 {COMP}/rof2elf.py {work}/src.obj {work}/src.elf --isa=sh2"],
            capture_output=True)
        subprocess.run(
            ["bash", "-c",
             f"{CROSS}/sh-elf-objcopy -O binary -j .text {work}/src.elf {work}/t.bin"],
            capture_output=True)
        if not os.path.exists(f"{work}/t.bin"):
            return None, "objcopy produced no .text"
        return open(f"{work}/t.bin", "rb").read(), ""
    finally:
        shutil.rmtree(work, ignore_errors=True)


def verify_one(entry, want_diff=False):
    addr = entry["addr"]
    src = os.path.join(HERE, entry["src"])
    flags = entry.get("flags", "-optimize=1")
    code_end, end, _pool, _tg = fn.extent(addr)
    target = fn.ROM[addr:end]
    got, err = compile_text(src, flags)
    res = {"addr": addr, "name": entry.get("name", ""),
           "extent": end - addr, "code": code_end - addr}
    if got is None:
        res.update(exact=False, matched=0, note=f"COMPILE_FAIL: {err}")
        return res
    exact = got == target
    n = min(len(got), len(target))
    same = sum(got[i] == target[i] for i in range(n))
    res.update(exact=exact, matched=(end - addr) if exact else 0,
               byte_score=f"{same}/{max(len(got), len(target), 1)}")
    if not exact and want_diff:
        ti = _dis(target[:code_end - addr])
        gi = _dis(got[:code_end - addr])
        rows = []
        for i in range(max(len(ti), len(gi))):
            t = ti[i] if i < len(ti) else ""
            g = gi[i] if i < len(gi) else ""
            rows.append(f"{t:28s} | {g}")
        res["diff"] = rows
        res["target_hex"] = target.hex()
        res["got_hex"] = got.hex()
    return res


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("addrs", nargs="*")
    ap.add_argument("--diff", action="store_true")
    ap.add_argument("--jobs", type=int, default=4)
    a = ap.parse_args()
    cfg = tomllib.load(open(os.path.join(HERE, "splits.toml"), "rb"))
    funcs = [f for f in cfg.get("func", []) if f.get("status") == "matched"]
    if a.addrs:
        sel = {int(x, 16) for x in a.addrs}
        funcs = [f for f in funcs if f["addr"] in sel]
    if not funcs:
        print("no matched funcs selected")
        return 0

    with ThreadPoolExecutor(max_workers=a.jobs) as ex:
        results = list(ex.map(lambda f: verify_one(f, a.diff), funcs))
    results.sort(key=lambda r: r["addr"])

    ok = 0
    mbytes = 0
    for r in results:
        tag = "EXACT " if r["exact"] else "FAIL  "
        extra = "" if r["exact"] else f"  [{r.get('byte_score', '-')}] {r.get('note', '')}"
        print(f"  {tag} 0x{r['addr']:06x} {r['name']:<16s} ext 0x{r['extent']:<4x}{extra}")
        if r["exact"]:
            ok += 1
            mbytes += r["matched"]
        elif a.diff and "diff" in r:
            print(f"    target: {r['target_hex']}")
            print(f"    got:    {r['got_hex']}")
            for line in r["diff"]:
                print(f"    {line}")
    print(f"\n{ok}/{len(results)} matched, {mbytes} bytes verified")

    cache_path = os.path.join(HERE, "build_results.json")
    cache = {}
    if os.path.exists(cache_path):
        try:
            cache = json.load(open(cache_path))
        except Exception:
            cache = {}
    for r in results:
        cache[f"0x{r['addr']:06x}"] = {"exact": r["exact"],
                                       "matched": r["matched"],
                                       "extent": r["extent"]}
    json.dump(cache, open(cache_path, "w"), indent=1, sort_keys=True)
    return 0 if ok == len(results) else 1


if __name__ == "__main__":
    sys.exit(main())
