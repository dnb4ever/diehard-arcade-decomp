#!/usr/bin/env bash
# Run from repository root. Snapshot creation/integration has already run once.
set -euo pipefail
w=${P421_WORK:-work/p421}
cmake -S "$w/generated" -B "$w/build" -DSATURN_RUNTIME="$PWD/$w/runtime" -DSATURN_EXTRA="$PWD/$w/port/stv.cmake" -DSATURN_REQUIRE_SDL=ON -DCMAKE_PREFIX_PATH="${P421_SDL_PREFIX:-$PWD/port/run/P46/sdl3deb/x/usr}"
cmake --build "$w/build" --target saturn_stv -j"${JOBS:-6}"
for example in identity infinite_health frame_hud; do
 "$w/port/mods/build_mod.sh" "$w/port/mods/examples/$example" "$w/data/mods/${example//_/-}" "$w/build/p421-target.txt"
done
cmake -S "$w/runtime/tests" -B "$w/tests" -DSATURN_NO_SDL=ON
cmake --build "$w/tests" --target runtime_tests -j"${JOBS:-6}"
"$w/tests/runtime_tests"
