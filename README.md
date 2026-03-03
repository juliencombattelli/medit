# Med

My personal text editor, simple, the way I want it to work.

## How to use

TODO

## How to build

### Install dependencies

Medit uses the following external dependencies:
- SDL 3.4+ and SDL_ttf 3.2+

You can install them with your system package manager if it provides a
compatible version.

Alternativelly you can build them from source and install them locally.
If so, you may have to install additional software for your platform.

#### SDL dependencies

For SDL libraries:
- Linux: https://github.com/libsdl-org/SDL/blob/release-3.4.2/docs/README-linux.md
- Windows: https://github.com/libsdl-org/SDL/blob/release-3.4.2/docs/README-windows.md

Doc for other platforms at https://github.com/libsdl-org/SDL/blob/release-3.4.2/docs

### Easy native build

Medit provides the script `./scripts/build.sh` to easily build the project along
with its dependencies.

Note that as SDL will be built from source, you may have to follow the
instructions to install [its own build dependencies](#sdl-dependencies).
