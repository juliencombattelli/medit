#!/bin/sh

CONFIG="$1"

FETCHCONTENT_QUIET=NO

case $CONFIG in 
    '')
        echo "CONFIG not set, falling back to RelWithDebInfo."
        CONFIG=RelWithDebInfo
        ;;
    Release)
        FETCHCONTENT_QUIET=YES
        ;;
esac

cmake -S . -B build -G"Ninja Multi-Config" -DFETCHCONTENT_QUIET=$FETCHCONTENT_QUIET
cmake --build build --config "$CONFIG"

# cmake -S . -B _build/windows-msvc -G"Ninja Multi-Config" -DFETCHCONTENT_QUIET=$FETCHCONTENT_QUIET \
#     --toolchain cmake/zig/x86_64-windows-msvc.cmake
# cmake --build _build/windows-msvc --config "$CONFIG"
