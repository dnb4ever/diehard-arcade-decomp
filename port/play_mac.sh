#!/bin/bash
# P4.8: launcher by default; --no-launcher starts the game immediately.
set -euo pipefail
cd "$(dirname "$0")/.."
for v in ${!STV_@}; do unset "$v"; done
BIN=port/run/P48/build-window/saturn_stv
if [ ! -x "$BIN" ]; then
    echo 'Build first: port/p48_build_mac.sh' >&2
    exit 2
fi
mkdir -p port/run/P48/shots
exec "$BIN" --cart port/diehard_main.bin --cartrom cart.bin --window --launcher \
    --settings port/run/P48/stv.ini --out port/run/P48/shots "$@"
