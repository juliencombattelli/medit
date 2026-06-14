# Hot-reloadable apps are separated into two parts:
# - the application code embedded in a reloadable dynamically-linked library
# - the main executable acting as the loader
#
# Main executable
# It contains the main entry point and platform-specific logic to (re)load the
# application library.
# It embeds the entire static SDL/SDL_ttf archives into the host (which is never
# reloaded) and export their symbols so the reloadable module resolves them from
# this process. Even if the application library links to those libs, as the
# executable exports the symbols we won't have duplicated symbols while loading
# the module.
#
# Reloadable app library
# Contains the compiled application code.
# Windows requires to explicitly link the dependencies (SDL and co) in order to
# produce a .lib file along with the .dll file. We link the dependencies on all
# platform as exporting the symbols from the main executable avoids having
# duplicated symbols.

option(PLAYGROUND_HOT_RELOAD_ENABLE "Support hot-reloading the playground hot-reload app" ON)

if(PLAYGROUND_HOT_RELOAD_ENABLE AND NOT CMAKE_LINK_LIBRARY_USING_WHOLE_ARCHIVE_SUPPORTED)
    message(WARNING "Linking a whole archive is not supported by the currently used linker, disabling hot-reloading.")
    set(PLAYGROUND_HOT_RELOAD_ENABLE OFF)
endif()

set(PLAYGROUND_APP_LINK_LIBS
    SDL3::SDL3 SDL3_ttf::SDL3_ttf utf8proc::utf8proc
)

if (PLAYGROUND_HOT_RELOAD_ENABLE)
    set(APP_LIB_MODE MODULE)
else()
    set(APP_LIB_MODE STATIC)
endif()

add_library(playground_hot_reloadable_app ${APP_LIB_MODE} app.c)
if (PLAYGROUND_HOT_RELOAD_ENABLE)
    target_compile_definitions(playground_hot_reloadable_app PUBLIC
        HOT_RELOAD_ENABLE
    )
endif()
target_compile_options(playground_hot_reloadable_app PRIVATE ${COMPILE_OPTIONS})
target_include_directories(playground_hot_reloadable_app PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(playground_hot_reloadable_app PRIVATE
    ${PLAYGROUND_APP_LINK_LIBS}
)

add_executable(playground_hot_reload main.c)
target_compile_options(playground_hot_reload PRIVATE ${COMPILE_OPTIONS})
if (PLAYGROUND_HOT_RELOAD_ENABLE)
    target_compile_definitions(playground_hot_reload PUBLIC
        HOT_RELOAD_ENABLE
    )
    target_link_libraries(playground_hot_reload PRIVATE
        $<LINK_LIBRARY:WHOLE_ARCHIVE, ${PLAYGROUND_APP_LINK_LIBS} >
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

install(TARGETS playground_hot_reload
    RUNTIME DESTINATION .
)
if(PLAYGROUND_HOT_RELOAD_ENABLE)
    install(TARGETS playground_hot_reloadable_app
        LIBRARY DESTINATION .
    )
endif()
install(TARGETS playground_hot_reload
    RUNTIME_DEPENDENCY_SET playground_hot_reload-deps
    RUNTIME DESTINATION .
)
install(RUNTIME_DEPENDENCY_SET playground_hot_reload-deps
    PRE_EXCLUDE_REGEXES
        ${MEDIT_PRE_EXCLUDE_REGEXES}
    POST_EXCLUDE_REGEXES
        ${MEDIT_POST_EXCLUDE_REGEXES}
    DESTINATION .
    DIRECTORIES ${MEDIT_TOOLCHAIN_DLL_PATH}
)
