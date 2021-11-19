cmake_minimum_required(VERSION 3.0.0)

project(RootTest LANGUAGES C)

# Creates a folder structure like this:
# install
# └── three
#     ├── libthree.so
#     └── two
#         ├── libtwo.so
#         └── one
#             └── one

# where one and libtwo have $ORIGIN/.. rpaths. This is used to test whether --root
# stops at root the boundary for relative rpaths.

# Disable global rpath settings
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH OFF)
set(CMAKE_INSTALL_RPATH "")
set(CMAKE_BUILD_WITH_INSTALL_RPATH OFF)

add_executable(one one.c)
add_library(two SHARED two.c)
add_library(three SHARED three.c)

# Make sure not to link to libc
target_link_options(one PRIVATE "-nostdlib")
target_link_options(two PRIVATE "-nostdlib")
target_link_options(three PRIVATE "-nostdlib")

target_link_libraries(one PRIVATE two)
target_link_libraries(two PRIVATE three)

install(TARGETS one RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/three/two/one")
install(TARGETS two LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/three/two")
install(TARGETS three LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/three")

set_target_properties(one PROPERTIES  INSTALL_RPATH "\$ORIGIN/..")
set_target_properties(two PROPERTIES  INSTALL_RPATH "\$ORIGIN/..")