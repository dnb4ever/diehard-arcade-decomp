#!/bin/bash
# Autolearn loop for the cold-boot + config-fixed path. Like autolearn_boot.sh but also
# writes the ST-V DIP/config word 0x06000798 (STV_CFG798, bit5 = normal-operation/proceed,
# bit6 = sound-enable) and optionally the region word 0x06000248 (STV_CFG248), which the
# harness vector-fill would otherwise leave as placeholder garbage. These steer the boot
# onto the real attract->credit->round path, which reaches round code (0604xxxx) that the
# attract-only default never did; this loop seeds the PIC-call walls that path hits.
# usage: autolearn_cold.sh [BOOTMODE] [MAX] [VB] [STVINPUT] [CFG798] [CFG248]
PORT=$HOME/decomp/diehard-stv/port
cd "$PORT"
BM=${1:-1}; MAX=${2:-40}; VB=${3:-4000}
INP=${4:-"200:COIN,260:COIN,340:START1,500:START1,900:START1,2000:START1,3000:START1"}
CFG798=${5:-20}; CFG248=${6:-}
ENVV="STV_BOOTMODE=$BM STV_CFG798=$CFG798"
[ -n "$CFG248" ] && ENVV="$ENVV STV_CFG248=$CFG248"
for i in $(seq 1 "$MAX"); do
  OUT=$(eval $ENVV timeout 600 ./recomp-build/saturn_stv --cart diehard_main.bin \
        --cartrom ../cart.bin --vblanks "$VB" --stvinput "\"$INP\"" --out run 2>&1)
  RC=$?
  ADDR=$(echo "$OUT" | grep -oE "call to [0-9A-Fa-f]{8}, not an entry of an active module" | head -1 | grep -oE "[0-9A-Fa-f]{8}" | head -1)
  if [ -z "$ADDR" ]; then
    echo "=== STOP at iter $i (rc=$RC): no learnable non-entry ==="
    echo "$OUT" | grep -iE "FATAL|STOP|stopped|VBlank limit|not an entry|slave vector|returned" | head -8
    exit 0
  fi
  if [ "$ADDR" = "00000000" ]; then
    echo "=== STOP at iter $i: next non-entry is 00000000 (data/garbage) ==="; exit 0
  fi
  echo "iter $i: learned seed $ADDR (bootmode $BM cfg798 $CFG798 cfg248 ${CFG248:-def})"
  echo "$ADDR  # learned (cold+cfg798=$CFG798${CFG248:+ cfg248=$CFG248})" >> seeds_extra.txt
  ./relearn.sh >/dev/null 2>&1
done
echo "=== hit iteration cap $MAX ==="
