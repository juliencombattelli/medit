#!/bin/sh

set -eu

usage() {
    echo "Install Medit dependencies at a desired place."
    echo
    echo "Usage:"
    echo "  setup.sh SRC_DIR [INSTALL_DIR]"
    echo
    echo "Arguments:"
    echo "  SRC_DIR         The path where to clone and build the projects."
    echo "  BUILD_DIR       The path where the build artifacts are stored."
    echo "  INSTALL_DIR     The path where to install the projects. Optional."
    echo "                    If not specified, a default value is used by CMake."
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
INSTALL_DIR="${3:-}"

if [ -z "$SRC_DIR" ]; then
    echo "Missing argument"
    usage
    exit 1
fi

if [ ! -d "$SRC_DIR" ]; then
    mkdir -p "$SRC_DIR"
fi

# Rely on "$@" for more robust word splitting
set --
if [ -n "$INSTALL_DIR" ]; then
    set -- "-DCMAKE_INSTALL_PREFIX=$INSTALL_DIR"
    if [ ! -d "$INSTALL_DIR" ]; then
        mkdir -p "$INSTALL_DIR"
    fi
fi

# Download dependencies
clone_or_checkout https://github.com/libsdl-org/SDL release-3.4.2 "$SRC_DIR/SDL"
clone_or_checkout https://github.com/libsdl-org/SDL_ttf release-3.2.2 "$SRC_DIR/SDL_ttf"
clone_or_checkout https://github.com/JuliaStrings/utf8proc v2.11.3 "$SRC_DIR/utf8proc"

CONFIG=RelWithDebInfo

# Build and install SDL (static)
cmake -S "$SRC_DIR/SDL" -B "$BUILD_DIR/SDL/build" \
    -G"Ninja Multi-Config" "$@" \
    -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_INSTALL_DOCS=ON
cmake --build "$BUILD_DIR/SDL/build" -j --config $CONFIG
cmake --install "$BUILD_DIR/SDL/build" --config $CONFIG

# Build and install SDL_ttf (static)
cmake -S "$SRC_DIR/SDL_ttf" -B "$BUILD_DIR/SDL_ttf/build" \
    -G"Ninja Multi-Config" "$@" \
    -DBUILD_SHARED_LIBS=OFF -DSDLTTF_INSTALL_MAN=ON -DSDLTTF_VENDORED=ON
cmake --build "$BUILD_DIR/SDL_ttf/build" -j --config $CONFIG
cmake --install "$BUILD_DIR/SDL_ttf/build" --config $CONFIG

# Build and install utf8proc (static)
cmake -S "$SRC_DIR/utf8proc" -B "$BUILD_DIR/utf8proc/build" \
    -G"Ninja Multi-Config" "$@" \
    -DBUILD_SHARED_LIBS=OFF -DUTF8PROC_ENABLE_TESTING=OFF
cmake --build "$BUILD_DIR/utf8proc/build" -j --config $CONFIG
cmake --install "$BUILD_DIR/utf8proc/build" --config $CONFIG
