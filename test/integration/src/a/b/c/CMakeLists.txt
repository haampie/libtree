cmake_minimum_required (VERSION 3.0)
project (c)

set(CMAKE_SKIP_BUILD_RPATH TRUE)

add_library(libc SHARED ${CMAKE_CURRENT_LIST_DIR}/libc.c)
target_link_libraries(libc PRIVATE libd)
set_target_properties(libc PROPERTIES LINK_FLAGS "-Wl,--disable-new-dtags,-rpath,\\$ORIGIN/../..")
