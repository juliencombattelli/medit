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
