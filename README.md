# Medit

My meditation on what editing should be.

Medit is my personal text editor, built from the ground up for simplicity and
performance. It reflects my opinionated views on editing: minimalist, fast, and
tailored exactly to my workflow. No bloat, no compromise, just what I need.

## How to use

TODO

## How to build

### Install dependencies

Medit uses the following external dependencies:
- SDL 3.4+ and SDL_ttf 3.2+

You can install them with your system package manager if it provides a
compatible version.

Alternatively, you can build them from source and install them locally.
If so, you may have to install additional software for your platform.

#### Convenience scripts

Medit provides the scripts `./scripts/build-<target>.sh` to easily fetch the
dependencies and build them before building the project for some targets.

Note that as SDL & Co will be built from source, you may still have to follow
the instructions to install [its own build dependencies](#sdl-dependencies).

The following targets are currently supported and well-tested by those scripts:
- Linux (native)
- LLVM-based MinGW (cross-compile from Linux)

The following targets were supported but are no longer used and maintained:
- MinGW

#### SDL dependencies

For SDL libraries, you will find the instructions to install all required
dependencies here:
- Linux: https://github.com/libsdl-org/SDL/blob/release-3.4.2/docs/README-linux.md
- Windows: https://github.com/libsdl-org/SDL/blob/release-3.4.2/docs/README-windows.md

Doc for other platforms at https://github.com/libsdl-org/SDL/blob/release-3.4.2/docs
