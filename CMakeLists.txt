cmake_minimum_required(VERSION 3.20)
project(Med C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
    sdl
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.2
    GIT_PROGRESS ON
)
set(SDL_STATIC ON)
set(SDL_SHARED OFF)
set(SDL_TEST_LIBRARY OFF)
set(SDL_TESTS OFF)
set(SDL_EXAMPLES OFF)

FetchContent_Declare(
    sdl_ttf
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
    GIT_TAG release-3.2.2
    GIT_PROGRESS ON
)
set(SDLTTF_VENDORED ON)
set(BUILD_SHARED_LIBS OFF) # Force static linking

FetchContent_MakeAvailable(sdl sdl_ttf)

add_executable(med src/main.c)

if(MSVC)
    set(WARNINGS /W4)
elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(WARNINGS -Wall -Wextra -Wshadow -Wconversion)
endif()

target_compile_options(med PRIVATE ${WARNINGS})

target_link_libraries(med PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf)
