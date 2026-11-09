#!/bin/bash
# Portable, CPU-only offline graphics and SCSP probes. Run from any directory.
set -eu
cd "$(dirname "$0")/.."
P411_ROOT=$PWD
mkdir -p work/p411/probe-source
cat > work/p411/probe-source/CMakeLists.txt <<'CMAKE'
cmake_minimum_required(VERSION 3.20)
project(p411_probes CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Release)
set(SATURN_NO_SDL ON)
include(${P411_ROOT}/tools/saturn-recomp/runtime/runtime.cmake)
foreach(probe render_dump scsp_probe)
  add_executable(p411_${probe} ${P411_ROOT}/port/p411_${probe}.cpp
    ${P411_ROOT}/tools/saturn-recomp/runtime/tests/no_modules.cpp)
  target_link_libraries(p411_${probe} saturn_core saturn_machine saturn_host Threads::Threads)
endforeach()
CMAKE
cmake -S work/p411/probe-source -B work/p411/probes -DP411_ROOT="$P411_ROOT"
cmake --build work/p411/probes -j "${JOBS:-8}"
