#!/usr/bin/env bash
# Build only the already frozen P4.20 experiment, never the live port/runtime.
set -euo pipefail
cd "$(dirname "$0")/.."
test -f work/p420/snapshot.json || { echo 'Missing work/p420 snapshot; see milestone report.' >&2; exit 1; }
cp port/p420_options.h port/p420_video.inc port/p420_present.inc port/p420_test_video.cpp work/p420/port/
cmake -S work/p420/port/run/P41/recomp -B work/p420/build-final \
  -DSATURN_RUNTIME="$PWD/work/p420/saturn-recomp/runtime" \
  -DSATURN_EXTRA="$PWD/work/p420/port/stv.cmake" \
  -DCMAKE_PREFIX_PATH="$PWD/work/p420/port/run/P46/sdl3deb/x/usr"
cmake --build work/p420/build-final --target saturn_stv p420_video_test -j6
