cmake_minimum_required(VERSION 3.21)
project(Medit C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(cmake/FindPackageOrFetch/SDL3.cmake)
include(cmake/FindPackageOrFetch/SDL3_ttf.cmake)
include(cmake/FindPackageOrFetch/utf8proc.cmake)

if(MSVC)
    set(COMPILE_OPTIONS /W4)
    set(LINK_OPTIONS)
elseif(CMAKE_C_COMPILER_ID MATCHES ".*Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(COMPILE_OPTIONS
        -Wall -Wextra -Wshadow -Wconversion -pedantic
        -Wno-missing-braces
    )
    set(LINK_OPTIONS)

    # Sanitizers are not supported by all toolchains (especially MinGW), so check if it works.
    # TODO add an option to disable it, and enable it automatically for Debug and ReleaseWithDebInfo builds
    include(CheckSourceCompiles)
    set(_sanitizer_flags -fsanitize=address,undefined -fno-omit-frame-pointer)
    set(CMAKE_REQUIRED_FLAGS ${_sanitizer_flags})
    set(CMAKE_REQUIRED_LINK_OPTIONS ${_sanitizer_flags})
    check_source_compiles(C "int main(void) { return 0; }" HAVE_ASAN_UBSAN)
    if(HAVE_ASAN_UBSAN)
        set(COMPILE_OPTIONS ${COMPILE_OPTIONS} ${_sanitizer_flags})
        set(LINK_OPTIONS ${LINK_OPTIONS} ${_sanitizer_flags})
    endif()
    unset(_sanitizer_flags)
endif()

add_executable(medit
    src/main.c src/meditor.c src/keybind.c src/unicode.c src/utils.c
    # SDL3 UI
    src/ui/sdl3/sdl3.c src/ui/sdl3/font.c src/ui/sdl3/keybind_sdl3.c src/ui/sdl3/perf_counter.c
)
target_compile_options(medit PRIVATE ${COMPILE_OPTIONS})
target_link_options(medit PRIVATE ${LINK_OPTIONS})
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
    set(MEDIT_PRE_EXCLUDE_REGEXES
        [[api-ms-.*]] [[ext-ms-.*]]
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
install(RUNTIME_DEPENDENCY_SET medit-deps
    PRE_EXCLUDE_REGEXES
        ${MEDIT_PRE_EXCLUDE_REGEXES}
    POST_EXCLUDE_REGEXES
        ${MEDIT_POST_EXCLUDE_REGEXES}
    DESTINATION .
    DIRECTORIES ${MEDIT_TOOLCHAIN_DLL_PATH}
)
install(DIRECTORY asset DESTINATION .)
# Install sources as well just to be able to open them on Windows
install(DIRECTORY src DESTINATION .)

add_executable(test_dynarray test/dynarray.c)
target_compile_options(test_dynarray PRIVATE ${COMPILE_OPTIONS})
target_link_options(test_dynarray PRIVATE ${LINK_OPTIONS})
target_include_directories(test_dynarray PRIVATE src)

add_executable(test_layout test/layout.c)
target_compile_options(test_layout PRIVATE ${COMPILE_OPTIONS})
target_link_options(test_layout PRIVATE ${LINK_OPTIONS})
target_include_directories(test_layout PRIVATE src)

add_executable(test_unicode test/unicode.c src/unicode.c)
target_compile_options(test_unicode PRIVATE ${COMPILE_OPTIONS})
target_link_options(test_unicode PRIVATE ${LINK_OPTIONS})
target_include_directories(test_unicode PRIVATE src)
target_link_libraries(test_unicode PRIVATE utf8proc::utf8proc)
