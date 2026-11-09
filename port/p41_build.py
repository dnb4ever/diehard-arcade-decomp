#!/usr/bin/env python3
"""Rebuild P4.1 in an isolated output tree, retaining previously learned entries."""
import os
import argparse
from pathlib import Path
import re
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--generate-only', action='store_true', help='emit C++ without configuring or building')
parser.add_argument('--headless', action='store_true', help='build the null host even when SDL3 is installed')
args = parser.parse_args()

root = Path(__file__).resolve().parent.parent
out = root / "port/run/P41"
out.mkdir(parents=True, exist_ok=True)
table = (root / "port/recomp/p_dh_table.cpp").read_text()
seeds = {int(x, 16) for x in re.findall(r"\{0x([0-9A-Fa-f]+)u,", table)}
for line in (root / "notes/symbols.txt").read_text().splitlines():
    match = re.match(r"\s*0x([0-9A-Fa-f]{8})\s", line)
    if match:
        seeds.add(int(match[1], 16) + 0x03E04000)
for line in (root / "port/seeds_extra.txt").read_text().splitlines():
    line = line.split("#")[0].strip()
    if line:
        seeds.add(int(line, 16))
entries = [f"{seed:08X}" for seed in sorted(seeds)]
(out / "combined-seeds.txt").write_text("\n".join(entries) + "\n")
spec = f"dh={root}/port/diehard_main.bin@06004000+" + ",".join(entries)
subprocess.run(
    ["python3", "-m", "saturnrecomp.recomp", "--out", str(out / "recomp"), spec],
    cwd=root / "tools/saturn-recomp", env=dict(os.environ, PYTHONPATH="src"), check=True,
)
subprocess.run(["python3", str(root / "port/p410_continuations.py"), str(out / "recomp")], check=True)
subprocess.run(["python3", str(root / "port/p415_idle_loops.py"), str(out / "recomp")], check=True)
if args.generate_only:
    raise SystemExit(0)
subprocess.run([
    "cmake", "-S", str(out / "recomp"), "-B", str(out / "build-generated"),
    f"-DSATURN_RUNTIME={root}/tools/saturn-recomp/runtime",
    f"-DSATURN_EXTRA={root}/port/stv.cmake",
    f"-DSATURN_NO_SDL={'ON' if args.headless else 'OFF'}",
], check=True)
subprocess.run([
    "cmake", "--build", str(out / "build-generated"),
    "--target", "saturn_stv", "-j4",
], check=True)
