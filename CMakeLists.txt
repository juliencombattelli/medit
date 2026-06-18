cmake_minimum_required(VERSION 3.24)
project(Medit C)

################################################################################
### Options
################################################################################

# Address sanitizer
# AUTO enables ASan only in Debug and RelWithDebInfo
set(MEDIT_ENABLE_ASAN "AUTO" CACHE STRING "Enable sanitizers (AUTO, ON, OFF)")
set_property(CACHE MEDIT_ENABLE_ASAN PROPERTY STRINGS AUTO ON OFF)

################################################################################

include(cmake/Modules/CheckPlatformSupported.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_ttf CONFIG REQUIRED)
find_package(utf8proc CONFIG REQUIRED)

if(MSVC)
    set(COMPILE_OPTIONS /W4)
elseif(CMAKE_C_COMPILER_ID MATCHES ".*Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(COMPILE_OPTIONS
        -Wall -Wextra -Wshadow -Wconversion -Wpedantic
        -Wno-missing-braces
    )
endif()

add_subdirectory(vendor/clay)

add_executable(medit
    # Editor entry point
    src/main.c src/default_config.c src/medit.c
    # Loader (TODO remove)
    src/loader/loader.c
    # Core modules
    src/core/meditor.c src/core/keybind.c src/core/unicode.c src/core/utils.c
    src/core/rect.c src/core/action.c src/core/ui.c src/core/string_view.c
    # SDL3 UI modules
    src/ui/sdl3/sdl3.c
    src/ui/sdl3/sdl3_renderer.c src/ui/sdl3/sdl3_layout.c src/ui/sdl3/sdl3_actions.c
    src/ui/sdl3/sdl3_core.c src/ui/sdl3/sdl3_font.c src/ui/sdl3/sdl3_file_view.c
    src/ui/sdl3/sdl3_tab_bar.c src/ui/sdl3/sdl3_keybind.c
    # SDL3 utils modules
    src/ui/sdl3/utils/perf_counter.c
    src/ui/sdl3/utils/font.c
)
target_compile_options(medit PRIVATE ${COMPILE_OPTIONS})
target_include_directories(medit PRIVATE src)
target_link_libraries(medit PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf utf8proc::utf8proc)

if(UNIX)
    set_target_properties(medit PROPERTIES
        INSTALL_RPATH "$ORIGIN"
    )
endif()

install(TARGETS medit RUNTIME DESTINATION .)
if(WIN32)
    # TARGET_RUNTIME_DLLS resolves DLLs of all CMake-known imported shared-library
    # targets linked to the executable, regardless of whether they came from a
    # system install, a manual install (SDL3_DIR=...), or FetchContent.
    install(FILES $<TARGET_RUNTIME_DLLS:medit> DESTINATION .)
elseif(UNIX AND NOT APPLE)
    # We cannot use $<TARGET_SONAME_FILE:...> directly because install(FILES)
    # copies symlinks as symlinks, which breaks in a relocated directory.
    # Instead we install the real versioned file under the SONAME name.
    foreach(_dep SDL3::SDL3 SDL3_ttf::SDL3_ttf utf8proc::utf8proc)
        install(FILES $<TARGET_FILE:${_dep}>
            DESTINATION .
            RENAME $<TARGET_SONAME_FILE_NAME:${_dep}>
        )
    endforeach()
endif()
install(DIRECTORY asset DESTINATION .)
# Install sources as well just to be able to open them on Windows for playing around
install(DIRECTORY src DESTINATION .)

include(CTest)

add_executable(test_dynarray test/dynarray.c)
target_compile_options(test_dynarray PRIVATE ${COMPILE_OPTIONS})
target_include_directories(test_dynarray PRIVATE src)
add_test(NAME Dynarray COMMAND test_dynarray)

add_executable(test_layout test/layout.c)
target_compile_options(test_layout PRIVATE ${COMPILE_OPTIONS})
target_include_directories(test_layout PRIVATE src)
add_test(NAME Layout COMMAND test_layout)

add_executable(test_unicode test/unicode.c src/core/unicode.c)
target_compile_options(test_unicode PRIVATE ${COMPILE_OPTIONS})
target_include_directories(test_unicode PRIVATE src)
target_link_libraries(test_unicode PRIVATE utf8proc::utf8proc)
add_test(NAME Unicode COMMAND test_unicode)

add_executable(test_string_view test/string_view.c src/core/string_view.c)
target_compile_options(test_string_view PRIVATE ${COMPILE_OPTIONS})
target_include_directories(test_string_view PRIVATE src)
add_test(NAME StringView COMMAND test_string_view)

install(
    TARGETS
        test_dynarray test_layout test_unicode test_string_view
    RUNTIME DESTINATION ./test
)

add_subdirectory(playground/hot-reload)
add_subdirectory(playground/ui)
add_subdirectory(playground/ui-clay)

# Sanitizers are not supported by all toolchains (especially MinGW), so check if it works
include(cmake/Modules/CheckAddressSanitizerSupported.cmake)
check_asan_supported(
    RESULT HAVE_ASAN
    COMPILE_OPTIONS ASAN_COMPILE_OPTIONS
    LINK_OPTIONS ASAN_LINK_OPTIONS
)
if(MEDIT_ENABLE_ASAN STREQUAL "ON" AND NOT HAVE_ASAN)
    message(WARNING
        "MEDIT_ENABLE_ASAN is set to ON but the current compiler/linker does not support AddressSanitizer "
        "(${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}, ${CMAKE_C_COMPILER}). "
        "AddressSanitizer will not be enabled."
    )
elseif(HAVE_ASAN)
    # Enable AddressSanitizer for all targets built if requested
    foreach(_target medit test_dynarray test_layout test_unicode test_string_view playground_ui_clay)
        if(MEDIT_ENABLE_ASAN STREQUAL "ON")
            target_compile_options(${_target} PRIVATE ${ASAN_COMPILE_OPTIONS})
            target_link_options(${_target} PRIVATE ${ASAN_LINK_OPTIONS})
        elseif(MEDIT_ENABLE_ASAN STREQUAL "AUTO")
            target_compile_options(${_target} PRIVATE
                $<$<CONFIG:Debug,RelWithDebInfo>:${ASAN_COMPILE_OPTIONS}>
            )
            target_link_options(${_target} PRIVATE
                $<$<CONFIG:Debug,RelWithDebInfo>:${ASAN_LINK_OPTIONS}>
            )
        endif()
    endforeach()
endif()
