#!/bin/bash
# ISOLATED scene-probe autolearn for dh-portA. Inherits STV_* env from the caller (so the
# scene-director probe knobs are active), runs saturn_stv, and on a "call to ADDR, not an
# entry of an active module" FATAL, appends ADDR to port/seeds_extra.txt and regenerates
# (relearn_portA.sh). Repeats until no learnable wall or the iteration cap. dh-portA only.
PORT=$HOME/decomp/dh-portA/port
cd "$PORT"
MAX=${AL_MAX:-30}
VB=${AL_VB:-4000}
EXTRA_ARGS=${AL_ARGS:---stvinput "100:COIN,160:COIN"}
for i in $(seq 1 "$MAX"); do
  mkdir -p run/al_scene
  OUT=$(timeout 600 ./recomp-build/saturn_stv --cart diehard_main.bin --cartrom ../cart.bin \
        --vblanks "$VB" $EXTRA_ARGS --out run/al_scene 2>&1)
  ADDR=$(echo "$OUT" | grep -oE "call to [0-9A-Fa-f]{8}, not an entry of an active module" | head -1 | grep -oE "^call to [0-9A-Fa-f]{8}" | grep -oE "[0-9A-Fa-f]{8}")
  if [ -z "$ADDR" ]; then
    echo "=== STOP at iter $i: no learnable non-entry ==="
    echo "$OUT" | grep -iE "FATAL|STOP|VBlank limit|not an entry|did not rte" | head -6
    exit 0
  fi
  if [ "$ADDR" = "00000000" ]; then
    echo "=== STOP at iter $i: next non-entry is 00000000 (data/garbage) ==="
    exit 0
  fi
  if grep -qi "^$ADDR" seeds_extra.txt; then
    echo "=== STOP at iter $i: $ADDR already seeded (loop) ==="
    echo "$OUT" | grep -iE "FATAL|not an entry" | head -4
    exit 0
  fi
  echo "iter $i: learned seed $ADDR"
  echo "$ADDR  # learned (scene-probe)" >> seeds_extra.txt
  ./relearn_portA.sh >/dev/null 2>&1
done
echo "=== hit iteration cap $MAX ==="
