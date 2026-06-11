add_executable(playground_ui_clay
    main.c clay_utils.c clay_sdl3.c sdl3_ext.c
    ${PROJECT_SOURCE_DIR}/src/default_settings.c
)
target_compile_options(playground_ui_clay PRIVATE ${COMPILE_OPTIONS})
target_include_directories(playground_ui_clay PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(playground_ui_clay PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf utf8proc::utf8proc)

install(
    TARGETS
        playground_ui_clay
    RUNTIME DESTINATION .
)
