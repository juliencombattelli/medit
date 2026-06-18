include_guard(GLOBAL)

#[=======================================================================[.rst:
Loader
------

.. command:: add_hot_reloadable_executable

  .. code-block:: cmake

    add_hot_reloadable_executable(
        <target_exe_name>
        <target_lib_name>
    )

  Creates two CMake targets:

  - ``<target_exe_name>``: the main executable (loader / entry point).
  - ``<target_lib_name>``: the hot-reloadable application library — built as a
    MODULE when hot-reload is enabled, STATIC otherwise.

  Also creates a CMake cache option::

    <TARGET_EXE_NAME_UPPER>_HOT_RELOAD_ENABLE (default ON)

  Set it to ``OFF`` to build the app code statically linked into the executable
  instead of loading it at runtime.

  Source files, link libraries, compile options, etc. can be added after this
  call using the usual ``target_*`` functions.
#]=======================================================================]
function(add_hot_reloadable_executable _exe _lib)

    # ---- Argument parsing ---------------------------------------------------

    # ---- Cache option -------------------------------------------------------

    string(TOUPPER "${_exe}" _upper)
    option(${_upper}_HOT_RELOAD_ENABLE "Enable hot-reloading for ${_exe}" ON)

    # ---- App library --------------------------------------------------------

    if(${_upper}_HOT_RELOAD_ENABLE)
        set(_lib_type MODULE)
    else()
        set(_lib_type STATIC)
    endif()

    add_library(${_lib} ${_lib_type})

    if(${_upper}_HOT_RELOAD_ENABLE)
        # PUBLIC so the definition propagates transitively when the lib is
        # linked statically (OFF path). In MODULE mode it is redundant but
        # keeps the pattern uniform.
        target_compile_definitions(${_lib} PUBLIC MEDIT_HOT_RELOAD_ENABLED)

        if(UNIX AND NOT APPLE)
            set_target_properties(${_lib} PROPERTIES
                INSTALL_RPATH "$ORIGIN"
            )
        endif()
    endif()

    # ---- Executable ---------------------------------------------------------

    add_executable(${_exe}
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../loader.c
    )

    if(${_upper}_HOT_RELOAD_ENABLE)
        target_compile_definitions(${_exe} PRIVATE
            MEDIT_HOT_RELOAD_ENABLED
            # $<TARGET_FILE_NAME:...> expands to the platform-correct filename
            # (e.g. libfoo.so / foo.dll) without any path component, matching
            # what the loader expects to load the module file.
            MEDIT_LOADER_APP_LIBRARY_NAME="$<TARGET_FILE_NAME:${_lib}>"
        )
        # Export all symbols so the MODULE library resolves them from this
        # process rather than loading duplicate copies of shared libraries.
        set_target_properties(${_exe} PROPERTIES
            ENABLE_EXPORTS ON
            WINDOWS_EXPORT_ALL_SYMBOLS ON
        )
    else()
        # Static path: embed the app library directly into the executable.
        target_link_libraries(${_exe} PRIVATE ${_lib})
    endif()

    # INSTALL_RPATH lets the installed binary find shared libraries placed next
    # to it in the install tree.
    # BUILD_RPATH_USE_ORIGIN is intentionally NOT set: CMake then keeps the
    # build-tree RPATH as absolute paths, allowing the binary to be run directly
    # from the build tree during development.
    if(UNIX AND NOT APPLE)
        set_target_properties(${_exe} PROPERTIES
            INSTALL_RPATH "$ORIGIN"
        )
    endif()
endfunction()
