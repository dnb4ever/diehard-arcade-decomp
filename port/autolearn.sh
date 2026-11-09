#!/bin/bash
# Auto learn loop: run the harness; if it dies on a "call to ADDR, not an entry of an
# active module", add ADDR as a seed and regenerate; repeat until it stops for another
# reason or hits the iteration cap. Prints each learned seed.
#
# Runs WITH the A-bus cartridge ROM mapped (--cartrom ../cart.bin) so the game reads its
# real sound/sequence/asset tables and the indirect call targets are genuine functions
# (not 0xFFFFFFFF garbage). VB must be high enough to reach the deep init faults (the
# sound-sequence setup faults around VBlank 10), so default VB is 60, not 3.
PORT=$HOME/decomp/diehard-stv/port
cd "$PORT"
MAX=${1:-40}
VB=${2:-60}
for i in $(seq 1 "$MAX"); do
  OUT=$(timeout 600 ./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin --vblanks "$VB" --out run 2>&1)
  RC=$?
  ADDR=$(echo "$OUT" | grep -oE "call to [0-9A-Fa-f]{8}, not an entry of an active module" | head -1 | grep -oE "[0-9A-Fa-f]{8}" | head -1)
  if [ -z "$ADDR" ]; then
    echo "=== STOP at iter $i (rc=$RC): no learnable non-entry ==="
    echo "$OUT" | grep -iE "FATAL|STOP|stopped|VBlank limit|not an entry|slave vector|returned|did not rte" | head -8
    exit 0
  fi
  # Guard: a null/garbage target is not a function to seed; stop and report.
  if [ "$ADDR" = "00000000" ]; then
    echo "=== STOP at iter $i: next non-entry is 00000000 (data/garbage, not code) ==="
    echo "$OUT" | grep -iE "FATAL|not an entry" | head -4
    exit 0
  fi
  echo "iter $i: learned seed $ADDR"
  echo "$ADDR  # learned (cartrom)" >> seeds_extra.txt
  ./relearn.sh >/dev/null 2>&1
done
echo "=== hit iteration cap $MAX ==="
