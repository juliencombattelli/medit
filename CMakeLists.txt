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
# Disable unwanted subsystems
set(SDL_AUDIO OFF)
set(SDL_JOYSTICK OFF)
set(SDL_HAPTIC OFF)
set(SDL_HIDAPI OFF)
set(SDL_SENSOR OFF)
set(SDL_POWER OFF)
set(SDL_DIALOG OFF)
set(SDL_TRAY OFF)
# Disable unwanted targets
set(SDL_TEST_LIBRARY OFF)
set(SDL_TESTS OFF)
set(SDL_EXAMPLES OFF)
# Force static linking
set(SDL_STATIC ON)
set(SDL_SHARED OFF)

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
