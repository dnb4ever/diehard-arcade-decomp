#!/usr/bin/env bash
# P4.17: isolated Linux x86_64 release, official SDL3, glibc 2.31 link sysroot.
set -euo pipefail
cd "$(dirname "$0")/.."
W=$PWD/work/p417
mkdir -p "$W/opt"
# P4.18: snapshot the integrated current tree, including final P4.16 netplay.
# P4.17 repo/runtime changes are already applied; never patch a stale copy.
python3 port/p410_continuations.py port/run/P41/recomp
python3 port/p415_idle_loops.py port/run/P41/recomp
python3 port/p422_prepare.py
python3 port/p418_stage_linux.py
python3 port/p417_package_appimage.py --fetch
# P4.22: qualified deployment already contains the original continuations and idle pass.
cmake --fresh -S "$W/opt/SDL3-3.4.20" -B "$W/build-sdl" \
    -DCMAKE_TOOLCHAIN_FILE="$W/toolchain.cmake" -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$W/opt/sdl" -DSDL_SHARED=ON -DSDL_STATIC=OFF \
    -DSDL_TESTS=OFF -DSDL_TEST_LIBRARY=OFF -DSDL_EXAMPLES=OFF \
    -DSDL_X11=ON -DSDL_WAYLAND=ON -DSDL_ALSA=ON -DSDL_PULSEAUDIO=ON \
    -DSDL_PIPEWIRE=OFF -DSDL_KMSDRM=OFF -DSDL_OPENGL=ON -DSDL_OPENGLES=ON \
    -DWAYLAND_SCANNER="$W/opt/sysroot/usr/bin/wayland-scanner"
cmake --build "$W/build-sdl" -j "${JOBS:-6}"
cmake --install "$W/build-sdl"
cmake --fresh -S "$W/recomp" -B "$W/build-linux" \
    -DCMAKE_TOOLCHAIN_FILE="$W/toolchain.cmake" -DCMAKE_BUILD_TYPE=Release \
    -DSATURN_RUNTIME="$W/saturn-recomp/runtime" -DSATURN_EXTRA="$W/saturn-recomp/port/stv.cmake" \
    -DSDL3_DIR="$W/opt/sdl/lib/cmake/SDL3" -DSATURN_NO_SDL=OFF -DSATURN_REQUIRE_SDL=ON \
    -DSATURN_STV_X86_V2=ON -DSATURN_STV_LINUX_RELEASE=ON \
    -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON '-DCMAKE_INSTALL_RPATH=$ORIGIN/../lib'
cmake --build "$W/build-linux" --target saturn_stv -j "${JOBS:-6}"
python3 port/p417_package_appimage.py
du -sh "$W"
