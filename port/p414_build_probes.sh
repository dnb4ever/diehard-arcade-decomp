#!/usr/bin/env bash
# CPU-only offline P4.14 compositor; no generated game or graphics API required.
set -euo pipefail
cd "$(dirname "$0")/.."
P414_ROOT=$PWD
mkdir -p work/p414/probe-source
cat > work/p414/probe-source/CMakeLists.txt <<'CMAKE'
cmake_minimum_required(VERSION 3.20)
project(p414_probes CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Release)
set(SATURN_NO_SDL ON)
include(${P414_ROOT}/tools/saturn-recomp/runtime/runtime.cmake)
add_executable(p414_render_dump ${P414_ROOT}/port/p414_render_dump.cpp
    ${P414_ROOT}/tools/saturn-recomp/runtime/tests/no_modules.cpp)
target_link_libraries(p414_render_dump saturn_core saturn_machine saturn_host Threads::Threads)
CMAKE
cmake -S work/p414/probe-source -B work/p414/probes -DP414_ROOT="$P414_ROOT"
cmake --build work/p414/probes -j "${JOBS:-8}"
