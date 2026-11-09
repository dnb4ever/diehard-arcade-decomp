#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p work/p415/probe-source
cat > work/p415/probe-source/CMakeLists.txt <<'CMAKE'
cmake_minimum_required(VERSION 3.20)
project(p415_probes CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Release)
include(${SATURN_RUNTIME}/runtime.cmake)
if(SDL3_FOUND)
  add_executable(p415_test_hidpi ${REPO}/port/p415_test_hidpi.cpp)
  target_include_directories(p415_test_hidpi PRIVATE ${SATURN_RUNTIME}/src/host)
  target_link_libraries(p415_test_hidpi SDL3::SDL3 saturn_imgui)
endif()
add_executable(p414_render_dump ${REPO}/port/p414_render_dump.cpp ${SATURN_RUNTIME}/tests/no_modules.cpp)
target_link_libraries(p414_render_dump saturn_core saturn_machine saturn_host Threads::Threads)
CMAKE
cmake -S work/p415/probe-source -B work/p415/probes -DSATURN_RUNTIME="$PWD/tools/saturn-recomp/runtime" -DREPO="$PWD" -DCMAKE_PREFIX_PATH="$PWD/port/run/P46/sdl3deb/x/usr"
cmake --build work/p415/probes -j "${JOBS:-6}"
