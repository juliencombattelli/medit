add_library(clay INTERFACE)
add_library(Clay::Clay ALIAS clay)
target_include_directories(clay INTERFACE ${CMAKE_CURRENT_LIST_DIR})
