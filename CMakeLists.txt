cmake_minimum_required(VERSION 3.20)
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
        $<$<NOT:$<BOOL:${MINGW}>>:-fsanitize=address,undefined>
        $<$<NOT:$<BOOL:${MINGW}>>:-fno-omit-frame-pointer>
    )
    set(LINK_OPTIONS
        $<$<NOT:$<BOOL:${MINGW}>>:-fsanitize=address,undefined>
    )
endif()

add_executable(medit
    src/main.c src/meditor.c src/keybind.c src/unicode.c
    # SDL3 UI
    src/ui/sdl3/sdl3.c src/ui/sdl3/font.c src/ui/sdl3/keybind_sdl3.c src/ui/sdl3/perf_counter.c
)
target_compile_options(medit PRIVATE ${COMPILE_OPTIONS})
target_link_options(medit PRIVATE ${LINK_OPTIONS})
target_include_directories(medit PRIVATE src)
target_link_libraries(medit PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf utf8proc::utf8proc)

if(WIN32)
    install(TARGETS medit RUNTIME DESTINATION .)
    install(RUNTIME_DEPENDENCY_SET deps
        PRE_EXCLUDE_REGEXES  [[api-ms-win-.*]] [[ext-ms-.*]]
        POST_EXCLUDE_REGEXES [[.*[/\\]system32[/\\].*\.dll]]
        DESTINATION .
    )
    install(DIRECTORY asset DESTINATION .)
    # Install sources as well just to be able to open them on Windows
    install(DIRECTORY src DESTINATION .)
endif()

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
