# ST-V cart-boot harness target, added to the generated build via -DSATURN_EXTRA.
# Headless (links the null host when SDL is absent); reuses saturn_machine wholesale.
add_executable(saturn_stv ${CMAKE_CURRENT_LIST_DIR}/main_stv.cpp)
target_include_directories(saturn_stv PRIVATE
    ${SATURN_RUNTIME}/include
    ${SATURN_RUNTIME}/src/machine
    ${SATURN_RUNTIME}/src/video
    ${SATURN_RUNTIME}/src/sound
    ${SATURN_RUNTIME}/src/host)
target_link_libraries(saturn_stv saturn_core saturn_machine saturn_host recomp Threads::Threads)

# Keep release ISA portable: SSE4.2/POPCNT on Linux/Windows x64, no AVX/AVX2.
# macOS retains its plain x86_64 deployment baseline. No fast-math or FMA changes.
option(SATURN_STV_X86_V2 "Use x86-64-v2 for Linux/Windows x64 releases" ON)
if(NOT APPLE AND CMAKE_SYSTEM_NAME MATCHES "Linux|Windows" AND
   CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$" AND
   CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND SATURN_STV_X86_V2)
    foreach(_target recomp saturn_core saturn_machine saturn_host saturn_m68k saturn_imgui saturn_stv)
        if(TARGET ${_target})
            target_compile_options(${_target} PRIVATE "$<$<CONFIG:Release>:-march=x86-64-v2>")
        endif()
    endforeach()
endif()

# P4.17 is enabled only by the Linux release builder.
option(SATURN_STV_LINUX_RELEASE "Linux release ROM browser and Steam defaults" OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND SATURN_STV_LINUX_RELEASE)
    target_sources(saturn_stv PRIVATE ${CMAKE_CURRENT_LIST_DIR}/p413_roms.c)
    target_compile_definitions(saturn_stv PRIVATE SATURN_P417_RELEASE=1)
    target_compile_definitions(saturn_host PRIVATE SATURN_P417_RELEASE=1)
    list(APPEND _p417_sources "${CMAKE_CURRENT_LIST_DIR}/p417_linux_frontend.h"
         "${CMAKE_CURRENT_LIST_DIR}/p413_roms.c" "${CMAKE_CURRENT_LIST_DIR}/p413_roms.h")
endif()

# Reject quick states from a different runtime, generated program or serializer.
file(GLOB_RECURSE _state_sources CONFIGURE_DEPENDS
    "${SATURN_RUNTIME}/src/*.cpp" "${SATURN_RUNTIME}/src/*.h"
    "${SATURN_RUNTIME}/src/*.c" "${SATURN_RUNTIME}/src/*.inc" "${SATURN_RUNTIME}/include/*.h")
list(APPEND _state_sources ${_p417_sources} "${CMAKE_CURRENT_LIST_DIR}/main_stv.cpp"
     "${CMAKE_CURRENT_LIST_DIR}/p410_continuations.py"
     "${CMAKE_CURRENT_LIST_DIR}/p415_idle_loops.py"
     "${CMAKE_CURRENT_LIST_FILE}" "${SATURN_RUNTIME}/runtime.cmake")
file(GLOB _state_generated "${CMAKE_CURRENT_SOURCE_DIR}/p_dh_*.cpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/p410_continuations.cpp")
list(APPEND _state_sources ${_state_generated})
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${_state_sources})
set(_state_hashes "${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}-${CMAKE_SYSTEM_PROCESSOR}-${CMAKE_SIZEOF_VOID_P}-${SATURN_STV_X86_V2}-${CMAKE_CXX_FLAGS}-${CMAKE_CXX_FLAGS_RELEASE}")
foreach(_state_source IN LISTS _state_sources)
    file(SHA256 "${_state_source}" _state_hash)
    string(APPEND _state_hashes "${_state_hash}")
endforeach()
set(_net_hashes "p416-wire-v1")
foreach(_net_source IN LISTS _state_sources)
    if(_net_source MATCHES "/(machine|core|video|sound)/.*\\.(cpp|h|c|inc)$" OR
       _net_source MATCHES "/p_dh_.*\\.cpp$" OR _net_source MATCHES "/p410_continuations.cpp$" OR _net_source MATCHES "/include/.*\\.h$")
        file(READ "${_net_source}" _net_text)
        # The Windows portability patch adds this standard include only.
        string(REPLACE "#include <cstdlib>\n" "" _net_text "${_net_text}")
        string(SHA256 _net_hash "${_net_text}")
        string(APPEND _net_hashes "${_net_hash}")
    endif()
endforeach()
string(SHA256 _net_identity "${_net_hashes}")
string(SUBSTRING "${_net_identity}" 0 16 _net_short)
string(SHA256 _state_identity "${_state_hashes}")
string(SUBSTRING "${_state_identity}" 0 16 _state_short)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/p410_identity.h" "#define STV_STATE_BUILD 0x${_state_short}ull\n#define STV_NETPLAY_CODE 0x${_net_short}ull\n")
target_include_directories(saturn_stv PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

# Original P4.13 app icon (Windows resource compiler; Linux/macOS target unchanged).
if(WIN32)
    enable_language(RC)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/p413_icon.rc"
         "1 ICON \"${CMAKE_CURRENT_LIST_DIR}/assets/p413-icon.ico\"\n")
    target_sources(saturn_stv PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/p413_icon.rc")
endif()
