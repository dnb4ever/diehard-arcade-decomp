#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
test -f work/p423/snapshot.json
cp port/p423_options.h port/p423_video.inc port/p423_present.inc port/p423_vdp2.inc port/p423_test_video.cpp work/p423/port/
cmake -S work/p423/port/run/P41/recomp -B work/p423/build \
  -DSATURN_RUNTIME="$PWD/work/p423/saturn-recomp/runtime" \
  -DSATURN_EXTRA="$PWD/work/p423/port/stv.cmake" \
  -DCMAKE_PREFIX_PATH="$PWD/work/p423/port/run/P46/sdl3deb/x/usr"
cmake --build work/p423/build --target saturn_stv p423_video_test -j6
