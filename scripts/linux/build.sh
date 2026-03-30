#!/bin/sh

./scripts/linux/setup.sh ../medit-deps ../medit-deps-linux-build ../medit-deps-linux-install

echo "Downloading fonts..."
mkdir -p asset/font
if [ ! -f asset/font/OpenMoji-color-colr0_svg.ttf ]; then
    wget https://github.com/hfg-gmuend/openmoji/raw/16.0.0/font/OpenMoji-color-colr0_svg/OpenMoji-color-colr0_svg.ttf \
        --output-document=asset/font/OpenMoji-color-colr0_svg.ttf --timeout=10
fi
if [ ! -f asset/font/Inconsolata-Regular.ttf ]; then
    wget https://github.com/google/fonts/raw/refs/heads/main/ofl/inconsolata/static/Inconsolata-Regular.ttf \
        --output-document=asset/font/Inconsolata-Regular.ttf --timeout=10
fi
if [ ! -f asset/font/consola.ttf ]; then
    if [ -f /mnt/c/Windows/Fonts/consola.ttf ]; then
        cp /mnt/c/Windows/Fonts/consola.ttf asset/font/consola.ttf
    fi
fi

cmake -S . -B build/linux \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo -DFETCHCONTENT_QUIET=NO \
    -DSDL3_DIR=../medit-deps-linux-install/lib/cmake/SDL3 \
    -DSDL3_ttf_DIR=../medit-deps-linux-install/lib/cmake/SDL3_ttf \
    -Dutf8proc_DIR=../medit-deps-linux-install/lib/cmake/utf8proc \

ln -sf build/linux/compile_commands.json .

cmake --build build/linux -j

# Cross compilation to Windows MSVC using Zig (not working at the moment)
# cmake -S . -B _build/windows-msvc -G"Ninja Multi-Config" -DFETCHCONTENT_QUIET=$FETCHCONTENT_QUIET \
#     --toolchain cmake/zig/x86_64-windows-msvc.cmake
# cmake --build _build/windows-msvc --config "$CONFIG"
