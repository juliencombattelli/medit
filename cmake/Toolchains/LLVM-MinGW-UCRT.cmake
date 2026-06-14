# Build for Windows using Clang and MinGW from a POSIX environment

# Requires a LLVM-MinGW toolchain
# Download it from: https://github.com/mstorsjo/llvm-mingw

# Unlike the MinGW toolchain, this LLVM-MinGW-UCRT toolchain allows using ASan and UBSan while also
# being a bit smaller on disk.

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER ${LLVM_MINGW_UCRT_DIR}/bin/clang)
set(CMAKE_CXX_COMPILER ${LLVM_MINGW_UCRT_DIR}/bin/clang++)
set(CMAKE_RC_COMPILER ${LLVM_MINGW_UCRT_DIR}/bin/llvm-windres)

set(CMAKE_C_COMPILER_TARGET x86_64-w64-mingw32)
set(CMAKE_CXX_COMPILER_TARGET x86_64-w64-mingw32)
set(CMAKE_RC_COMPILER_TARGET x86_64-w64-mingw32)

# CMake determines how to examine dependencies based on the *host* system, leading to
# a `file unknown error` unless the target platform is explicitly specified.
set(CMAKE_GET_RUNTIME_DEPENDENCIES_PLATFORM "windows+pe")
set(CMAKE_GET_RUNTIME_DEPENDENCIES_TOOL "objdump")

# Workaround for the LLVM MinGW toolchain that may not support linker arguments
# --{push,pop}-state but CMake tries to use them anyway.
# LLVM issue: https://github.com/llvm/llvm-project/issues/131007
# CMake issue: https://gitlab.kitware.com/cmake/cmake/-/work_items/27822
# This workaround works only with CMake 4.2 onward.
cmake_minimum_required(VERSION 4.2)
set(CACHE{CMAKE_LINKER_PUSHPOP_STATE_SUPPORTED} TYPE INTERNAL VALUE 0)
set(CACHE{CMAKE_C_LINKER_PUSHPOP_STATE_SUPPORTED} TYPE INTERNAL VALUE 0)
set(CACHE{CMAKE_CXX_LINKER_PUSHPOP_STATE_SUPPORTED} TYPE INTERNAL VALUE 0)
