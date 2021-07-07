cmake_minimum_required (VERSION 3.0)
project (a)

set(CMAKE_SKIP_BUILD_RPATH TRUE)

add_subdirectory(b)

add_library(liba SHARED ${CMAKE_CURRENT_LIST_DIR}/liba.c)
target_link_libraries(liba PRIVATE libb)
set_target_properties(liba PROPERTIES LINK_FLAGS "-Wl,--disable-new-dtags,-rpath,\\$ORIGIN/b")

add_library(libd SHARED ${CMAKE_CURRENT_LIST_DIR}/libd.c)
