#!/bin/bash
# Regenerate the recompiled C++ with the base symbol seeds + the learned extra seeds,
# then rebuild saturn_stv. The learn loop: run -> note a "not an entry" address ->
# append it to seeds_extra.txt -> relearn.sh -> run again.
set -e
PORT=$HOME/decomp/diehard-stv/port
SK=$HOME/decomp/diehard-stv/tools/saturn-recomp
cd "$SK"
SPEC=$(python3 - <<PY
import re
seeds=set()
for line in open("$HOME/decomp/diehard-stv/notes/symbols.txt"):
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
