cmake_minimum_required(VERSION 3.20)
project(Medit C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(cmake/FindPackageOrFetchSDL3.cmake)
include(cmake/FindPackageOrFetchSDL3_ttf.cmake)

add_executable(medit
    src/main.c src/meditor.c src/keybind.c src/renderer.c
    src/renderer_sdl3.c src/keybind_sdl3.c
)

if(MSVC)
    set(WARNINGS /W4)
elseif(CMAKE_C_COMPILER_ID MATCHES ".*Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(WARNINGS -Wall -Wextra -Wshadow -Wconversion -pedantic)
endif()

target_compile_options(medit PRIVATE ${WARNINGS})

target_link_libraries(medit PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf)

target_compile_options(medit PRIVATE -fsanitize=address,undefined)
target_link_options(medit PRIVATE -fsanitize=address,undefined)
