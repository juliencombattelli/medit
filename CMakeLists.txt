cmake_minimum_required(VERSION 3.20)
project(Medit C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(cmake/FindPackageOrFetchSDL3.cmake)
include(cmake/FindPackageOrFetchSDL3_ttf.cmake)

if(MSVC)
    set(COMPILE_OPTIONS /W4)
    set(LINK_OPTIONS)
elseif(CMAKE_C_COMPILER_ID MATCHES ".*Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(COMPILE_OPTIONS
        -Wall -Wextra -Wshadow -Wconversion -pedantic
        -Wno-missing-braces
        -fsanitize=address,undefined -fno-omit-frame-pointer
    )
    set(LINK_OPTIONS -fsanitize=address,undefined)
endif()

add_executable(medit
    src/main.c src/meditor.c src/keybind.c 
    # src/renderer.c
    # src/platform/sdl3/renderer_sdl3.c
    src/platform/sdl3/keybind_sdl3.c
    src/ui/sdl3/sdl3.c src/ui/sdl3/font.c
    # src/platform/ansi-terminal/renderer_ansi_term.c
    )
target_compile_options(medit PRIVATE ${COMPILE_OPTIONS})
target_link_options(medit PRIVATE ${LINK_OPTIONS})
target_include_directories(medit PRIVATE src)
target_link_libraries(medit PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf)

add_executable(test_dynarray test/dynarray.c)
target_compile_options(test_dynarray PRIVATE ${COMPILE_OPTIONS})
target_link_options(test_dynarray PRIVATE ${LINK_OPTIONS})
target_include_directories(test_dynarray PRIVATE src)
