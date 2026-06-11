add_executable(playground_ui
    main.c ui2.c ${PROJECT_SOURCE_DIR}/src/default_settings.c
)
target_compile_options(playground_ui PRIVATE ${COMPILE_OPTIONS})
target_include_directories(playground_ui PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(playground_ui PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf utf8proc::utf8proc)
