#!/bin/bash
# P4.11 v1.00: standalone "Die Hard Arcade.app" (Intel macOS, SDL3 bundled, no ROMs).
# Separate build tree from P4.8 so the app can target macOS 14 (Homebrew SDL3's floor).
set -eo pipefail
cd "$(dirname "$0")/.."
if [ "$(uname -s)" != Darwin ] || [ "$(uname -m)" != x86_64 ]; then
    echo 'P4.11 v1.00 requires Intel macOS (x86_64).' >&2
    exit 2
fi
if [ ! -f port/run/P41/recomp/CMakeLists.txt ] ||
   [ port/seeds_extra.txt -nt port/run/P41/recomp/CMakeLists.txt ] ||
   [ port/diehard_main.bin -nt port/run/P41/recomp/CMakeLists.txt ]; then
    python3 port/p41_build.py --generate-only
fi
python3 port/p410_continuations.py port/run/P41/recomp
python3 port/p415_idle_loops.py port/run/P41/recomp
SDL_PREFIX=${SDL3_PREFIX:-$(brew --prefix sdl3)}
if [ ! -f "$SDL_PREFIX/lib/cmake/SDL3/SDL3Config.cmake" ]; then
    echo 'SDL3 development files missing; set SDL3_PREFIX to an existing SDL3 install.' >&2
    exit 2
fi
export MACOSX_DEPLOYMENT_TARGET=${MACOSX_DEPLOYMENT_TARGET:-14.0}
cmake -S port/run/P41/recomp -B port/run/P411/build-app -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_OSX_DEPLOYMENT_TARGET="$MACOSX_DEPLOYMENT_TARGET" \
    -DSATURN_RUNTIME="$PWD/tools/saturn-recomp/runtime" -DSATURN_EXTRA="$PWD/port/stv.cmake" \
    -DSATURN_NO_SDL=OFF -DSATURN_REQUIRE_SDL=ON "-DCMAKE_PREFIX_PATH=$SDL_PREFIX"
cmake --build port/run/P411/build-app --target saturn_stv -j "${JOBS:-8}"
python3 port/p411_bundle_mac.py --binary port/run/P411/build-app/saturn_stv \
    --sdl-prefix "$SDL_PREFIX" --min-macos "$MACOSX_DEPLOYMENT_TARGET"
