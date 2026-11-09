#!/bin/bash
# P4.8 v1.00: Apple clang, Intel macOS, and the existing Homebrew SDL3.
set -eo pipefail
cd "$(dirname "$0")/.."
if [ "$(uname -s)" != Darwin ] || [ "$(uname -m)" != x86_64 ]; then
    echo 'P4.8 v1.00 requires Intel macOS (x86_64).' >&2
    exit 2
fi
MODE=window
case "${1:-}" in
    --headless) MODE=headless ;;
    '') ;;
    *) echo 'usage: port/p48_build_mac.sh [--headless]' >&2; exit 2 ;;
esac
if [ ! -f port/run/P41/recomp/CMakeLists.txt ] ||
   [ port/seeds_extra.txt -nt port/run/P41/recomp/CMakeLists.txt ] ||
   [ port/diehard_main.bin -nt port/run/P41/recomp/CMakeLists.txt ]; then
    python3 port/p41_build.py --generate-only
fi
python3 port/p410_continuations.py port/run/P41/recomp
python3 port/p415_idle_loops.py port/run/P41/recomp
PREFIX=()
NO_SDL=ON
REQUIRE_SDL=OFF
if [ "$MODE" = window ]; then
    SDL_PREFIX=${SDL3_PREFIX:-$(brew --prefix sdl3)}
    if [ ! -f "$SDL_PREFIX/lib/cmake/SDL3/SDL3Config.cmake" ]; then
        echo 'SDL3 development files missing; set SDL3_PREFIX to an existing SDL3 install.' >&2
        exit 2
    fi
    PREFIX=("-DCMAKE_PREFIX_PATH=$SDL_PREFIX")
    NO_SDL=OFF
    REQUIRE_SDL=ON
fi
cmake -S port/run/P41/recomp -B "port/run/P48/build-$MODE" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_OSX_ARCHITECTURES=x86_64 -DSATURN_RUNTIME="$PWD/tools/saturn-recomp/runtime" \
    -DSATURN_EXTRA="$PWD/port/stv.cmake" -DSATURN_NO_SDL="$NO_SDL" \
    -DSATURN_REQUIRE_SDL="$REQUIRE_SDL" "${PREFIX[@]}"
cmake --build "port/run/P48/build-$MODE" --target saturn_stv -j "${JOBS:-8}"
if [ "$MODE" = window ]; then
    python3 port/p48_bundle_mac.py
fi
