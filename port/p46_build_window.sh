#!/bin/bash
# P4.6: build the windowed saturn_stv (SDL3 host) into port/run/P46/build-sdl, from the generated
# recompile that port/p41_build.py writes to port/run/P41/recomp (run first if that is missing).
# SDL3: a system install if one is there; otherwise Ubuntu's libsdl3-dev unpacked without root into
# port/run/P46/sdl3deb (headers and CMake files only), linked to the system's libSDL3.so.0 (libsdl3-0).
set -e
cd "$(dirname "$0")/.."
R=$PWD
[ -d port/run/P41/recomp ] || python3 port/p41_build.py
python3 port/p410_continuations.py port/run/P41/recomp
python3 port/p415_idle_loops.py port/run/P41/recomp
PREFIX=()
if [ ! -f /usr/lib/x86_64-linux-gnu/cmake/SDL3/SDL3Config.cmake ]; then
    D=port/run/P46/sdl3deb
    L=$D/x/usr/lib/x86_64-linux-gnu
    if [ ! -f $L/cmake/SDL3/SDL3Config.cmake ]; then
        mkdir -p $D
        (cd $D && apt-get download libsdl3-dev && dpkg-deb -x libsdl3-dev_*.deb x)
    fi
    # the package's CMake files name the versioned library; point it (and the soname) at the system's
    SO=$(grep -o 'libSDL3\.so\.[0-9.]*[0-9]' $L/cmake/SDL3/SDL3sharedTargets-none.cmake | head -1)
    ln -sf "$(readlink -f /usr/lib/x86_64-linux-gnu/libSDL3.so.0)" $L/$SO
    ln -sf /usr/lib/x86_64-linux-gnu/libSDL3.so.0 $L/libSDL3.so.0
    PREFIX=(-DCMAKE_PREFIX_PATH=$R/$D/x/usr)
fi
cmake -S port/run/P41/recomp -B port/run/P46/build-sdl -DSATURN_RUNTIME=$R/tools/saturn-recomp/runtime \
    -DSATURN_EXTRA=$R/port/stv.cmake -DSATURN_REQUIRE_SDL=ON "${PREFIX[@]}"
cmake --build port/run/P46/build-sdl --target saturn_stv -j8
