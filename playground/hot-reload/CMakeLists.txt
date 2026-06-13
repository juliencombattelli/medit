# Hot-reloadable apps are separated into two parts:
# - the application code embedded in a reloadable dynamically-linked library
# - the main executable acting as the loader
#
# Main executable
# It contains the main entry point and platform-specific logic to (re)load the
# application library.
# It embeds the entire static SDL/SDL_ttf archives into the host (which is never
# reloaded) and export their symbols so the reloadable module resolves them from
# this process. This keeps one SDL instance (and its window/renderer) alive for
# the whole run, independent of reloads.
#
# Reloadable app library
# Contains the compiled application code.
# SDL/SDL_ttf are deliberately not linked into the reloadable module: because
# SDL is built statically, linking it here would embed a private copy of all of
# SDL's global state into the .so. dlclose() on hot-reload would then destroy
# that SDL instance (and the live window/renderer), and the freshly dlopen()ed
# module would get a new, uninitialized SDL copy holding stale handles.
# Instead we only pull in the headers and resolve the SDL symbols at load time
# from the host executable, so a single SDL instance survives across reloads.

option(PLAYGROUND_HOT_RELOAD_ENABLE "Support hot-reloading the playground hot-reload app" ON)

if(PLAYGROUND_HOT_RELOAD_ENABLE AND NOT CMAKE_LINK_LIBRARY_USING_WHOLE_ARCHIVE_SUPPORTED)
    message(WARNING "Linking a whole archive is not supported by the currently used linker, disabling hot-reloading.")
    set(PLAYGROUND_HOT_RELOAD_ENABLE OFF)
endif()

set(PLAYGROUND_APP_LINK_LIBS
     SDL3::SDL3 SDL3_ttf::SDL3_ttf utf8proc::utf8proc
)

if (PLAYGROUND_HOT_RELOAD_ENABLE)
    set(APP_LIB_MODE SHARED)
else()
    set(APP_LIB_MODE STATIC)
endif()
add_library(playground_hot_reloadable_app ${APP_LIB_MODE} app.c)
target_compile_options(playground_hot_reloadable_app PRIVATE ${COMPILE_OPTIONS})
target_include_directories(playground_hot_reloadable_app PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    $<TARGET_PROPERTY:SDL3::SDL3,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:SDL3_ttf::SDL3_ttf,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(playground_hot_reloadable_app PRIVATE
    $<TARGET_PROPERTY:SDL3::SDL3,INTERFACE_COMPILE_DEFINITIONS>
    $<TARGET_PROPERTY:SDL3_ttf::SDL3_ttf,INTERFACE_COMPILE_DEFINITIONS>
)
if (PLAYGROUND_HOT_RELOAD_ENABLE)
    target_compile_definitions(playground_hot_reloadable_app PUBLIC
        HOT_RELOAD_ENABLE
    )
endif()

add_executable(playground_hot_reload main.c)
target_compile_options(playground_hot_reload PRIVATE ${COMPILE_OPTIONS})
if (PLAYGROUND_HOT_RELOAD_ENABLE)
    target_compile_definitions(playground_hot_reload PUBLIC
        HOT_RELOAD_ENABLE
    )
    target_link_libraries(playground_hot_reload PRIVATE
        $<LINK_LIBRARY:WHOLE_ARCHIVE, ${PLAYGROUND_APP_LINK_LIBS} >
        dl
    )
    set_target_properties(playground_hot_reload PROPERTIES
        ENABLE_EXPORTS ON
        BUILD_RPATH ${CMAKE_CURRENT_BINARY_DIR}
    )
else()
    target_link_libraries(playground_hot_reload PRIVATE
        playground_hot_reloadable_app
        ${PLAYGROUND_APP_LINK_LIBS}
    )
endif()
