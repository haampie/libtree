cmake_minimum_required (VERSION 3.0)
project (bundler)

message(STATUS "Generating excludelist")
execute_process(
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/src/generate_exclude_list.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_subdirectory(deps)

add_executable(bundler src/main.cc)
target_link_libraries(bundler cxxopts termcolor stdc++fs cppglob ELFIO)
target_include_directories(bundler PRIVATE deps/cppglob/include ${CMAKE_CURRENT_BINARY_DIR})

set_property(TARGET bundler PROPERTY CXX_STANDARD 17)
