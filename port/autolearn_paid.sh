#!/bin/bash
# Autolearn on the PAID path (coin -> credit -> start). Seeds indirect-call walls that
# only the credited gameplay path reaches. Uses STV_PAIDTEST + a coin+start script.
PORT=$HOME/decomp/diehard-stv/port
cd "$PORT"
MAX=${1:-40}
VB=${2:-2000}
SCRIPT=${3:-"200:COIN,340:START1"}
for i in $(seq 1 "$MAX"); do
  OUT=$(timeout 600 ./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin --vblanks "$VB" --stvinput "$SCRIPT" --out run 2>&1)
  RC=$?
  ADDR=$(echo "$OUT" | grep -oE "call to [0-9A-Fa-f]{8}, not an entry of an active module" | head -1 | grep -oE "[0-9A-Fa-f]{8}" | head -1)
  if [ -z "$ADDR" ]; then
    echo "=== STOP at iter $i (rc=$RC): no learnable non-entry ==="
    echo "$OUT" | grep -iE "FATAL|STOP|stopped|VBlank limit|not an entry|slave vector|returned" | head -8
    exit 0
  fi
  if [ "$ADDR" = "00000000" ]; then
    echo "=== STOP at iter $i: next non-entry is 00000000 ==="
    echo "$OUT" | grep -iE "FATAL|not an entry" | head -4
    exit 0
  fi
  echo "iter $i: learned seed $ADDR"
  echo "$ADDR  # learned (paid path)" >> seeds_extra.txt
  ./relearn.sh >/dev/null 2>&1
done
echo "=== hit iteration cap $MAX ==="
