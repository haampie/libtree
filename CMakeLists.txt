cmake_minimum_required (VERSION 3.0)
project (libtree)

message(STATUS "Generating excludelist")
execute_process(
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/src/generate_exclude_list.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_subdirectory(deps)

set(SOURCE
   ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/deploy.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/deps.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/elf.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/exec.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/ld.cpp
)

add_executable(libtree ${SOURCE})
target_link_libraries(libtree cxxopts termcolor cppglob ELFIO)
target_include_directories(libtree PRIVATE include deps/cppglob/include ${CMAKE_CURRENT_BINARY_DIR})

set_property(TARGET libtree PROPERTY CXX_STANDARD 17)
