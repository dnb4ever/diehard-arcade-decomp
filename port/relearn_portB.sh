#!/bin/bash
# ISOLATED relearn for the dh-portB copy (does NOT touch dh-portA / diehard-stv / dh-film).
# Regenerates the recompiled C++ from notes/symbols.txt + port/seeds_extra.txt into
# dh-portB/port/recomp, then rebuilds saturn_stv in dh-portB/port/recomp-build.
# Mirror of relearn_portA.sh but all paths point at dh-portB.
set -e
ROOT=$HOME/decomp/dh-portB
PORT=$ROOT/port
SK=$ROOT/tools/saturn-recomp
cd "$SK"
SPEC=$(python3 - <<PY
import re
seeds=set()
for line in open("$ROOT/notes/symbols.txt"):
    m=re.match(r"\s*0x([0-9A-Fa-f]{8})\s", line)
    if m: seeds.add(int(m.group(1),16)+0x03E04000)
try:
    for line in open("$PORT/seeds_extra.txt"):
        line=line.split("#")[0].strip()
        if line: seeds.add(int(line,16))
except FileNotFoundError: pass
print("dh=$PORT/diehard_main.bin@06004000+"+",".join("%08X"%s for s in sorted(seeds)))
PY
)
PYTHONPATH=src python3 -m saturnrecomp.recomp --out "$PORT/recomp" $SPEC 2>&1 | tail -1
cmake --build "$PORT/recomp-build" --target saturn_stv -j"$(nproc)" 2>&1 | tail -1
