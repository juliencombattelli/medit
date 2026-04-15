#!/bin/bash

set -eu

VARIANT="$1"

export DEPS_SOURCE_DIR="../medit-deps"
export DEPS_BUILD_DIR="../medit-deps-build/$VARIANT"
export DEPS_INSTALL_DIR="../medit-deps-install/$VARIANT"
export MEDIT_BUILD_DIR="build/$VARIANT"

setup_deps() {
    usage() {
        echo "Install Medit dependencies at a desired place."
        echo
        echo "Usage:"
        echo "  setup-deps SRC_DIR BUILD_DIR [CMAKE_FLAGS]"
        echo
        echo "Arguments:"
        echo "  SRC_DIR         The path where to clone and build the projects."
        echo "  BUILD_DIR       The path where the build artifacts are stored."
        echo "  CMAKE_FLAGS     Additional flags passed to CMake. Optional."
    }

    clone_or_checkout() (
        url="$1"
        branch_or_tag="$2"
        src_dir="$3"
        if [ ! -d "$src_dir" ]; then
            git clone "$url" "$src_dir" -b "$branch_or_tag" --depth=1 --recursive
        else
            # TODO consider not doing a checkout to allow changes in dependencies
            git -C "$src_dir" checkout "$branch_or_tag"
        fi
    )

    SRC_DIR="$1"
    BUILD_DIR="$2"
    shift 2
    CMAKE_FLAGS=("$@")

    if [ -z "$SRC_DIR" ]; then
        echo "Missing argument"
        usage
        exit 1
    fi

    if [ ! -d "$SRC_DIR" ]; then
        mkdir -p "$SRC_DIR"
    fi

    # TODO using CMake in script mode to clone and build to allow sharing a maximum of data between the
    # shell script and the CMake build system

    echo "Downloading SDL v3.4.2"
    clone_or_checkout https://github.com/libsdl-org/SDL release-3.4.2 "$SRC_DIR/SDL"
    echo "Downloading SDL_ttf v3.2.2"
    clone_or_checkout https://github.com/libsdl-org/SDL_ttf release-3.2.2 "$SRC_DIR/SDL_ttf"
    echo "Downloading utf8proc v2.11.3"
    clone_or_checkout https://github.com/JuliaStrings/utf8proc v2.11.3 "$SRC_DIR/utf8proc"

    CONFIG=RelWithDebInfo

    echo "Building and installing SDL (static)"
    cmake -S "$SRC_DIR/SDL" -B "$BUILD_DIR/SDL/build" \
        -G"Ninja Multi-Config" "${CMAKE_FLAGS[@]}" \
        -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_INSTALL_DOCS=ON
    cmake --build "$BUILD_DIR/SDL/build" --parallel --config $CONFIG
    cmake --install "$BUILD_DIR/SDL/build" --config $CONFIG

    echo "Building and installing SDL_ttf (static)"
    cmake -S "$SRC_DIR/SDL_ttf" -B "$BUILD_DIR/SDL_ttf/build" \
        -G"Ninja Multi-Config" "${CMAKE_FLAGS[@]}" \
        -DBUILD_SHARED_LIBS=OFF -DSDLTTF_INSTALL_MAN=ON -DSDLTTF_VENDORED=ON
    cmake --build "$BUILD_DIR/SDL_ttf/build" --parallel --config $CONFIG
    cmake --install "$BUILD_DIR/SDL_ttf/build" --config $CONFIG

    echo "Building and installing utf8proc (static)"
    cmake -S "$SRC_DIR/utf8proc" -B "$BUILD_DIR/utf8proc/build" \
        -G"Ninja Multi-Config" "${CMAKE_FLAGS[@]}" \
        -DBUILD_SHARED_LIBS=OFF -DUTF8PROC_ENABLE_TESTING=OFF
    cmake --build "$BUILD_DIR/utf8proc/build" --parallel --config $CONFIG
    cmake --install "$BUILD_DIR/utf8proc/build" --config $CONFIG

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
}

configure_and_build() {
    # All the function arguments will be passed to CMake
    cmake -S . -B "$MEDIT_BUILD_DIR" \
        "$@" \
        -DCMAKE_BUILD_TYPE="RelWithDebInfo" -DFETCHCONTENT_QUIET=NO \
        -DSDL3_DIR="$DEPS_INSTALL_DIR/lib/cmake/SDL3" \
        -DSDL3_ttf_DIR="$DEPS_INSTALL_DIR/lib/cmake/SDL3_ttf" \
        -Dutf8proc_DIR="$DEPS_INSTALL_DIR/lib/cmake/utf8proc" \

    cmake --build "$MEDIT_BUILD_DIR" -j
}
