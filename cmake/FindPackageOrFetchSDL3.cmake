if (NOT DEFINED FETCHCONTENT_SOURCE_DIR_SDL3 AND DEFINED SDL3_DIR)
    # If the user set SDL3_DIR, then assume that they really want Medit to
    # use the specified installation, so error out if it is not found
    set(SDL3_REQUIRED REQUIRED)
endif()

find_package(SDL3 CONFIG ${SDL3_REQUIRED})

if(SDL3_FOUND)
    message(STATUS "Found SDL3 (SDL3_DIR: ${SDL3_DIR})")
else()
    message(STATUS "Using vendored SDL3")
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

    FetchContent_MakeAvailable(sdl)
endif()
