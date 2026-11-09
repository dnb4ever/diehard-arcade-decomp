# Linux-hosted llvm-mingw UCRT x64. Pass -DLLVM_MINGW_ROOT=/absolute/toolchain.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES LLVM_MINGW_ROOT)
if(NOT LLVM_MINGW_ROOT)
  message(FATAL_ERROR "Set LLVM_MINGW_ROOT to the unpacked llvm-mingw UCRT Linux toolchain")
endif()
set(CMAKE_C_COMPILER "${LLVM_MINGW_ROOT}/bin/x86_64-w64-mingw32-clang")
set(CMAKE_CXX_COMPILER "${LLVM_MINGW_ROOT}/bin/x86_64-w64-mingw32-clang++")
set(CMAKE_RC_COMPILER "${LLVM_MINGW_ROOT}/bin/x86_64-w64-mingw32-windres")
set(CMAKE_FIND_ROOT_PATH "${LLVM_MINGW_ROOT}/x86_64-w64-mingw32")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
# LLVM's integrated COFF assembler automatically emits bigobj as required.
# Do not pass GNU as's -Wa,-mbig-obj to clang's integrated assembler.
