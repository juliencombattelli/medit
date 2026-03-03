cmake_minimum_required(VERSION 3.20)
project(Med C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(SDL3 CONFIG)
if(NOT DEFINED SDL3_FOUND)
    include(cmake/FetchSDL3.cmake)
endif()

find_package(SDL3_ttf CONFIG)
if(NOT DEFINED SDL3_ttf_FOUND)
    include(cmake/FetchSDL3_ttf.cmake)
endif()

add_executable(med src/main.c)

if(MSVC)
    set(WARNINGS /W4)
elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(WARNINGS -Wall -Wextra -Wshadow -Wconversion)
endif()

target_compile_options(med PRIVATE ${WARNINGS})

target_link_libraries(med PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf)
