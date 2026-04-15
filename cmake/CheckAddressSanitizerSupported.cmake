include_guard(GLOBAL)

#[=======================================================================[.rst:
CheckAddressSanitizerSupported
------------------------------

.. command:: check_asan_supported

  .. code-block:: cmake

    check_asan_supported([RESULT <result>]
                         [COMPILE_OPTIONS <compile_options>]
                         [LINK_OPTIONS <link_options>])

  Check whether the current compiler/linker supports Address Sanitizer (and
  UBSan on non-MSVC toolchains).

  If called without ``RESULT``, a fatal error is raised when sanitizers are
  not supported. If ``RESULT`` is given, the variable is set to ``YES`` or
  ``NO`` in the calling scope.

  ``COMPILE_OPTIONS`` and ``LINK_OPTIONS`` are set to the required flags when
  sanitizers are supported, or empty when they are not.
#]=======================================================================]

function(check_asan_supported)
    cmake_parse_arguments(_ASAN "" "RESULT;COMPILE_OPTIONS;LINK_OPTIONS" "" ${ARGN})

    if(MSVC)
        set(_compile_flags /fsanitize=address)
        set(_link_flags)
    else()
        # Assume GCC-like command line interface
        # Also enable ubsan as it is generally available alongside asan
        set(_compile_flags -fsanitize=address,undefined -fno-omit-frame-pointer)
        set(_link_flags ${_compile_flags})
    endif()

    set(CMAKE_REQUIRED_FLAGS ${_compile_flags})
    set(CMAKE_REQUIRED_LINK_OPTIONS ${_link_flags})

    include(CheckSourceCompiles)
    check_source_compiles(C "int main(void) { return 0; }" _ASAN_SUPPORTED)

    if(_ASAN_RESULT)
        if(_ASAN_SUPPORTED)
            set(${_ASAN_RESULT} YES PARENT_SCOPE)
        else()
            set(${_ASAN_RESULT} NO PARENT_SCOPE)
        endif()
    endif()

    if(_ASAN_COMPILE_OPTIONS)
        if(_ASAN_SUPPORTED)
            set(${_ASAN_COMPILE_OPTIONS} ${_compile_flags} PARENT_SCOPE)
        else()
            set(${_ASAN_COMPILE_OPTIONS} "" PARENT_SCOPE)
        endif()
    endif()

    if(_ASAN_LINK_OPTIONS)
        if(_ASAN_SUPPORTED)
            set(${_ASAN_LINK_OPTIONS} ${_link_flags} PARENT_SCOPE)
        else()
            set(${_ASAN_LINK_OPTIONS} "" PARENT_SCOPE)
        endif()
    endif()
endfunction()
