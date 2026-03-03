#!/bin/sh

./scripts/setup.sh ../medit-deps ../medit-deps-install

cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo -DFETCHCONTENT_QUIET=NO \
    -DSDL3_DIR=../medit-deps-install/lib/cmake/SDL3 \
    -DSDL3_ttf_DIR=../medit-deps-install/lib/cmake/SDL3_ttf

cmake --build build -j

# Cross compilation to Windows MSVC using Zig (not working at the moment)
# cmake -S . -B _build/windows-msvc -G"Ninja Multi-Config" -DFETCHCONTENT_QUIET=$FETCHCONTENT_QUIET \
#     --toolchain cmake/zig/x86_64-windows-msvc.cmake
# cmake --build _build/windows-msvc --config "$CONFIG"
