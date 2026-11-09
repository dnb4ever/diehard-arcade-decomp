#!/bin/bash
# Autolearn loop for the BOOT-MODE (cold-init) path: like autolearn.sh but sets
# STV_BOOTMODE so the boot controller morphs func_022016a0 into the cold-boot start
# state (state 1 => func_0201b92 controller/sound/config init). Learns PIC call walls
# the cold-boot gameplay path reaches that the attract-only default never did.
# usage: autolearn_boot.sh [MODE] [MAX] [VB] [STVINPUT]
PORT=$HOME/decomp/diehard-stv/port
cd "$PORT"
MODE=${1:-1}; MAX=${2:-40}; VB=${3:-800}; INP=${4:-"200:COIN,340:START1"}
for i in $(seq 1 "$MAX"); do
  OUT=$(STV_BOOTMODE=$MODE timeout 600 ./recomp-build/saturn_stv --cart diehard_main.bin \
        --cartrom ../cart.bin --vblanks "$VB" --stvinput "$INP" --out run 2>&1)
  RC=$?
  ADDR=$(echo "$OUT" | grep -oE "call to [0-9A-Fa-f]{8}, not an entry of an active module" | head -1 | grep -oE "[0-9A-Fa-f]{8}" | head -1)
  if [ -z "$ADDR" ]; then
    echo "=== STOP at iter $i (rc=$RC): no learnable non-entry ==="
    echo "$OUT" | grep -iE "FATAL|STOP|stopped|VBlank limit|not an entry|slave vector|returned|did not rte" | head -8
    exit 0
  fi
  if [ "$ADDR" = "00000000" ]; then
    echo "=== STOP at iter $i: next non-entry is 00000000 (data/garbage) ==="
    echo "$OUT" | grep -iE "FATAL|not an entry" | head -4
    exit 0
  fi
  echo "iter $i: learned seed $ADDR (mode $MODE)"
  echo "$ADDR  # learned (bootmode $MODE)" >> seeds_extra.txt
  ./relearn.sh >/dev/null 2>&1
done
echo "=== hit iteration cap $MAX ==="
