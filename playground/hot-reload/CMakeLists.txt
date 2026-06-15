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

option(PLAYGROUND_HOT_RELOAD_ENABLE "Support hot-reloading the playground hot-reload app" ON)

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
    set_target_properties(playground_hot_reloadable_app PROPERTIES
        INSTALL_RPATH "\$ORIGIN"
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
    set_target_properties(playground_hot_reload PROPERTIES
        ENABLE_EXPORTS ON
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )
else()
    target_link_libraries(playground_hot_reload PRIVATE
        playground_hot_reloadable_app
    )
endif()
# $ORIGIN in INSTALL_RPATH lets the installed binary find shared libraries
# (SDL, etc.) placed next to it in the install directory, regardless of whether
# hot-reload is enabled.
# Note: BUILD_RPATH_USE_ORIGIN is intentionally NOT set. Without it, CMake
# keeps the build RPATH as absolute paths to the linked targets' directories.
# file(GET_RUNTIME_DEPENDENCIES) (used by RUNTIME_DEPENDENCY_SET) can then
# resolve SDL and co through the binary's RPATH directly, avoiding DIRECTORIES
# hints which cause a CMake warning.
set_target_properties(playground_hot_reload PROPERTIES
    INSTALL_RPATH "\$ORIGIN"
)
target_link_libraries(playground_hot_reload PRIVATE
    ${PLAYGROUND_APP_LINK_LIBS}
)

install(TARGETS playground_hot_reload
    RUNTIME DESTINATION .
)
if(PLAYGROUND_HOT_RELOAD_ENABLE)
    install(TARGETS playground_hot_reloadable_app
        LIBRARY DESTINATION .
    )
endif()

# Install the DLLs/SOs next to the main binary.
# TARGET_RUNTIME_DLLS is non-empty only on Windows. On non-Windows (aka Linux),
# RUNTIME_DEPENDENCY_SET resolves deps via the binary's embedded absolute RPATH,
# so DIRECTORIES only needs the toolchain path.
if(WIN32)
    install(FILES $<TARGET_RUNTIME_DLLS:playground_hot_reload> DESTINATION .)
else()
    install(TARGETS playground_hot_reload
        RUNTIME_DEPENDENCY_SET playground_hot_reload-deps
        RUNTIME DESTINATION .
    )
    install(RUNTIME_DEPENDENCY_SET playground_hot_reload-deps
        PRE_EXCLUDE_REGEXES ${MEDIT_PRE_EXCLUDE_REGEXES}
        POST_EXCLUDE_REGEXES ${MEDIT_POST_EXCLUDE_REGEXES}
        DESTINATION .
        DIRECTORIES ${MEDIT_TOOLCHAIN_DLL_PATH}
    )
endif()
