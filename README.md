# Med

My personal text editor, simple, the way I want it to work.

## How to use

TODO

## How to build

### Native build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo && cmake --build build --parallel
```

### Cross-compile build (using zig)

Configuration for a Windows build:
```bash
cmake -S . -B build-windows-gnu -DCMAKE_BUILD_TYPE=RelWithDebInfo --toolchain cmake/zig.cmake -DTARGET=x86_64-windows-gnu && cmake --build build --parallel
```
