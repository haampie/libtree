cmake_minimum_required (VERSION 3.0)
project (integration_test)

set(CMAKE_SKIP_BUILD_RPATH TRUE)

add_subdirectory(c)

add_library(libb SHARED ${CMAKE_CURRENT_LIST_DIR}/libb.c)
target_link_libraries(libb PRIVATE libc)
set_target_properties(libb PROPERTIES LINK_FLAGS "-Wl,--enable-new-dtags,-rpath,\\$ORIGIN/c")
