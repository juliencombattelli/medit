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
