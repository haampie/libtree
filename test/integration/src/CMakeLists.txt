cmake_minimum_required (VERSION 3.0)
project (main)

set(CMAKE_SKIP_BUILD_RPATH TRUE)

add_subdirectory(a)

add_executable(main ${CMAKE_CURRENT_LIST_DIR}/main.c)
target_link_libraries(main PRIVATE liba)
set_target_properties(main PROPERTIES LINK_FLAGS "-Wl,--disable-new-dtags,-rpath,\\$ORIGIN/a")
