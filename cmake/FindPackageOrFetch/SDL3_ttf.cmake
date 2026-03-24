
if (NOT DEFINED FETCHCONTENT_SOURCE_DIR_SDL3_ttf AND DEFINED SDL3_ttf_DIR)
    # If the user set SDL3_ttf_DIR, then assume that they really want Medit to
    # use the specified installation, so error out if it is not found
    set(SDL3_ttf_REQUIRED REQUIRED)
endif()

find_package(SDL3_ttf CONFIG ${SDL3_ttf_REQUIRED})

if(SDL3_ttf_FOUND)
    message(STATUS "Found SDL3_ttf (SDL3_ttf_DIR: ${SDL3_ttf_DIR})")
else()
    message(STATUS "Using vendored SDL3_ttf")
    include(FetchContent)

    FetchContent_Declare(
        sdl_ttf
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
        GIT_TAG release-3.2.2
        GIT_PROGRESS ON
    )
    set(SDLTTF_VENDORED ON)
    set(BUILD_SHARED_LIBS OFF) # Force static linking

    FetchContent_MakeAvailable(sdl_ttf)
endif()
