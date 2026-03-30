#!/bin/sh

./scripts/mingw/setup.sh ../medit-deps ../medit-deps-mingw-build ../medit-deps-mingw-install

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

cmake -S . -B build-mingw \
    --toolchain cmake/MinGW.cmake \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo -DFETCHCONTENT_QUIET=NO \
    -DSDL3_DIR=../medit-deps-mingw-install/lib/cmake/SDL3 \
    -DSDL3_ttf_DIR=../medit-deps-mingw-install/lib/cmake/SDL3_ttf \
    -Dutf8proc_DIR=../medit-deps-mingw-install/lib/cmake/utf8proc \

cmake --build build-mingw -j
