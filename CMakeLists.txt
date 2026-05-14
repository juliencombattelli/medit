cmake_minimum_required(VERSION 3.21)
project(Medit C)

################################################################################
### Options
################################################################################

# Address sanitizer
# AUTO enables ASan only in Debug and RelWithDebInfo
set(MEDIT_ENABLE_ASAN "AUTO" CACHE STRING "Enable sanitizers (AUTO, ON, OFF)")
set_property(CACHE MEDIT_ENABLE_ASAN PROPERTY STRINGS AUTO ON OFF)

################################################################################

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(cmake/Modules/FindPackageOrFetch/SDL3.cmake)
include(cmake/Modules/FindPackageOrFetch/SDL3_ttf.cmake)
include(cmake/Modules/FindPackageOrFetch/utf8proc.cmake)

if(MSVC)
    set(COMPILE_OPTIONS /W4)
elseif(CMAKE_C_COMPILER_ID MATCHES ".*Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(COMPILE_OPTIONS
        -Wall -Wextra -Wshadow -Wconversion -pedantic
        -Wno-missing-braces
    )
endif()

add_executable(medit
    # Editor entry point
    src/main.c src/default_settings.c
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

if(MINGW)
    if (CMAKE_C_COMPILER_ID MATCHES ".*Clang")
        # The following objdump trick is required when CMake (before v3.29) and LLVM's objdump are used
        # in combination: the objdump binary from LLVM uses 4 spaces before "DLL Name:" where other
        # objdump builds print a tab. CMake before v3.29 is only checking for tabs, leading to no DLL
        # being detected at all.
        # Refer to https://gitlab.kitware.com/cmake/cmake/-/merge_requests/9325
        # and https://gitlab.kitware.com/cmake/cmake/-/work_items/20753
        file(WRITE ${PROJECT_BINARY_DIR}/objdump_wrapper.sh
        "${CMAKE_OBJDUMP} $@ | sed 's/^    DLL Name:/\tDLL Name:/'"
        )
        file(CHMOD ${PROJECT_BINARY_DIR}/objdump_wrapper.sh
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
        )
        set(CMAKE_GET_RUNTIME_DEPENDENCIES_COMMAND ${PROJECT_BINARY_DIR}/objdump_wrapper.sh)

        # TODO don't assume x86_64 here
        set(MEDIT_TOOLCHAIN_DLL_PATH ${LLVM_MINGW_UCRT_DIR}/x86_64-w64-mingw32/bin)
    endif()

    # TODO check how installation goes with MSVC
    set(MEDIT_TOOLCHAIN_PRE_EXCLUDE_REGEXES
        # The following DLLs are not included in the LLVM-MinGW toolchain, so we exclude them
        # before resolution as they will therefore never be resolved to proper system32 DLLs and
        # would trigger a resolution error
        [[^advapi32\.dll$]]
        [[^gdi32\.dll$]]
        [[^imm32\.dll$]]
        [[^kernel32\.dll$]]
        [[^oleaut32\.dll$]]
        [[^rpcrt4\.dll$]]
        [[^setupapi\.dll$]]
        [[^shell32\.dll$]]
        [[^user32\.dll$]]
        [[^usp10\.dll$]]
        [[^version\.dll$]]
        [[^winmm\.dll$]]
        [[^ole32\.dll$]]
        [[^msvcrt\.dll$]]
    )
endif()

if(WIN32)
    if(MSVC)
        # Required to locate ASan DLL properly
        set(MEDIT_TOOLCHAIN_DLL_PATH $ENV{VCToolsInstallDir}/bin/Hostx86/x64)
    endif()
    set(MEDIT_PRE_EXCLUDE_REGEXES
        [[api-ms-.*]] [[ext-ms-.*]]
        [[vcruntime.*]] # installed via InstallRequiredSystemLibraries
        ${MEDIT_TOOLCHAIN_PRE_EXCLUDE_REGEXES}
    )
    set(MEDIT_POST_EXCLUDE_REGEXES
        [[.*[/\]system32[/\].*\.dll]]
    )
elseif(APPLE)
    set(MEDIT_POST_EXCLUDE_REGEXES
        [[.*\.framework.*]]
        [[/usr/lib/libSystem]]
        [[/usr/lib/libobjc]]
    )
    set_target_properties(medit PROPERTIES
        INSTALL_RPATH "@executable_path"
        MACOSX_RPATH ON
    )
elseif(UNIX)
    set(MEDIT_POST_EXCLUDE_REGEXES
        [[ld-linux]]          # glibc dynamic linker
        [[ld-musl]]           # musl dynamic linker
        [[/libc\.so]]         # C library
        [[/libm\.so]]         # math (always part of base system)
        [[/libdl\.so]]        # dlopen (glibc <2.34, or musl symlink)
        [[/libpthread\.so]]   # threads (glibc <2.34, or musl symlink)
        [[/librt\.so]]        # realtime (glibc <2.34, or musl symlink)
    )
    set_target_properties(medit PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH_USE_ORIGIN ON
    )
endif()

install(TARGETS medit
    RUNTIME_DEPENDENCY_SET medit-deps
    RUNTIME DESTINATION .
)
# Install the DLLs next to the main binary
set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".")
include(InstallRequiredSystemLibraries)
install(RUNTIME_DEPENDENCY_SET medit-deps
    PRE_EXCLUDE_REGEXES
        ${MEDIT_PRE_EXCLUDE_REGEXES}
    POST_EXCLUDE_REGEXES
        ${MEDIT_POST_EXCLUDE_REGEXES}
    DESTINATION .
    DIRECTORIES ${MEDIT_TOOLCHAIN_DLL_PATH}
)
install(DIRECTORY asset DESTINATION .)
# Install sources as well just to be able to open them on Windows for playing around
install(DIRECTORY src DESTINATION .)

include(CTest)

add_executable(test_dynarray test/dynarray.c)
target_compile_options(test_dynarray PRIVATE ${COMPILE_OPTIONS})
target_link_options(test_dynarray PRIVATE ${LINK_OPTIONS})
target_include_directories(test_dynarray PRIVATE src)
add_test(NAME Dynarray COMMAND test_dynarray)

add_executable(test_layout test/layout.c)
target_compile_options(test_layout PRIVATE ${COMPILE_OPTIONS})
target_link_options(test_layout PRIVATE ${LINK_OPTIONS})
target_include_directories(test_layout PRIVATE src)
add_test(NAME Layout COMMAND test_layout)

add_executable(test_unicode test/unicode.c src/core/unicode.c)
target_compile_options(test_unicode PRIVATE ${COMPILE_OPTIONS})
target_link_options(test_unicode PRIVATE ${LINK_OPTIONS})
target_include_directories(test_unicode PRIVATE src)
target_link_libraries(test_unicode PRIVATE utf8proc::utf8proc)
add_test(NAME Unicode COMMAND test_unicode)

add_executable(test_string_view test/string_view.c src/core/string_view.c)
target_compile_options(test_string_view PRIVATE ${COMPILE_OPTIONS})
target_link_options(test_string_view PRIVATE ${LINK_OPTIONS})
target_include_directories(test_string_view PRIVATE src)
add_test(NAME StringView COMMAND test_string_view)

install(
    TARGETS
        test_dynarray test_layout test_unicode test_string_view
    RUNTIME DESTINATION ./test
)

add_executable(playground_ui
    playground/ui/main.c playground/ui/ui2.c src/default_settings.c
)
target_compile_options(playground_ui PRIVATE ${COMPILE_OPTIONS})
target_include_directories(playground_ui PRIVATE src)
target_link_libraries(playground_ui PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf utf8proc::utf8proc)

add_executable(playground_ui_clay
    playground/ui-clay/main.c src/default_settings.c
)
target_compile_options(playground_ui_clay PRIVATE ${COMPILE_OPTIONS})
target_include_directories(playground_ui_clay PRIVATE src)
target_link_libraries(playground_ui_clay PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf utf8proc::utf8proc)

install(
    TARGETS
        playground_ui_clay
    RUNTIME DESTINATION .
)

# Sanitizers are not supported by all toolchains (especially MinGW), so check if it works
include(cmake/Modules/CheckAddressSanitizerSupported.cmake)
check_asan_supported(
    RESULT HAVE_ASAN
    COMPILE_OPTIONS ASAN_COMPILE_OPTIONS
    LINK_OPTIONS ASAN_LINK_OPTIONS
)
if(MEDIT_ENABLE_ASAN STREQUAL ON AND NOT HAVE_ASAN)
    message(WARNING
        "MEDIT_ENABLE_ASAN is set to ON but the current compiler/linker does not support AddressSanitizer "
        "(${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}, ${CMAKE_C_COMPILER}). "
        "AddressSanitizer will not be enabled."
    )
elseif(HAVE_ASAN)
    # Enable AddressSanitizer for all targets built if requested
    foreach(_target medit test_dynarray test_layout test_unicode)
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
