if (NOT DEFINED FETCHCONTENT_SOURCE_DIR_UTF8PROC AND DEFINED utf8proc_DIR)
    # If the user set utf8proc_DIR, then assume that they really want Medit to
    # use the specified installation, so error out if it is not found
    set(utf8proc_REQUIRED REQUIRED)
endif()

find_package(utf8proc CONFIG ${utf8proc_REQUIRED})

if(utf8proc_FOUND)
    message(STATUS "Found utf8proc (utf8proc_DIR: ${utf8proc_DIR})")
else()
    message(STATUS "Using vendored utf8proc")
    include(FetchContent)
    
    FetchContent_Declare(
        utf8proc
        GIT_REPOSITORY https://github.com/JuliaStrings/utf8proc.git
        GIT_TAG v2.11.3
        GIT_PROGRESS ON
    )

    # Disable unwanted targets
    set(UTF8PROC_ENABLE_TESTING OFF)

    # Force static linking
    set(BUILD_SHARED_LIBS OFF)

    FetchContent_MakeAvailable(utf8proc)

    # The utf8proc CMakeLists does not defined the alias target defined by their config file
    add_library(utf8proc::utf8proc ALIAS utf8proc)
endif()
