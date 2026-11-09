#!/bin/bash
# P4.6: play Die Hard Arcade in a window, in real time, with sound (keys: notes/port/p46_realtime.md).
# Extra arguments go to saturn_stv: --fullscreen, --scale N, --settings FILE, --stats SECONDS.
# Screenshots (F12) go to port/run/P46/shots.
cd "$(dirname "$0")/.."
for v in ${!STV_@}; do unset "$v"; done        # the canonical route: no harness knobs
mkdir -p port/run/P46/shots
exec port/run/P46/build-sdl/saturn_stv --cart port/diehard_main.bin --cartrom cart.bin --window \
    --out port/run/P46/shots "$@"
