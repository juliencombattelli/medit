# Hot-reloadable apps are separated into two parts:
# - the application code embedded in a reloadable dynamically-linked library
# - the main executable acting as the loader
#
# Main executable
# Contains the main entry point and platform-specific logic to (re)load the
# application library. It links dynamically to SDL and co, and exports their
# symbols so the reloadable module resolves them from this process rather than
# loading its own copies.
#
# Reloadable app library
# Contains the application code.
# It is built as a MODULE (dynamically loadable) when hot-reload is enabled, or
# as a STATIC library otherwise. It links to the same SDL targets on all
# platforms: on Windows this is required to produce the .lib import file
# alongside the .dll; on Linux/macOS the symbols are already exported by the
# main executable so no duplicate copies are loaded.

include(${PROJECT_SOURCE_DIR}/src/loader/cmake/Loader.cmake)

add_hot_reloadable_executable(
    playground_hot_reload
    playground_hot_reloadable_app
)

target_sources(playground_hot_reload PRIVATE main.c)
target_include_directories(playground_hot_reload PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)
target_compile_options(playground_hot_reload PRIVATE ${COMPILE_OPTIONS})
target_link_libraries(playground_hot_reload PRIVATE
    SDL3::SDL3 SDL3_ttf::SDL3_ttf utf8proc::utf8proc
)

target_sources(playground_hot_reloadable_app PRIVATE app.c)
target_compile_options(playground_hot_reloadable_app PRIVATE ${COMPILE_OPTIONS})
target_include_directories(playground_hot_reloadable_app PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(playground_hot_reloadable_app PRIVATE
    SDL3::SDL3 SDL3_ttf::SDL3_ttf utf8proc::utf8proc
)

install(TARGETS playground_hot_reload RUNTIME DESTINATION .)
if(PLAYGROUND_HOT_RELOAD_HOT_RELOAD_ENABLE)
    install(TARGETS playground_hot_reloadable_app
        LIBRARY DESTINATION .
    )
endif()
