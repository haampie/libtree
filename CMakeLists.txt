cmake_minimum_required (VERSION 3.0)
project (bundler)

add_subdirectory(deps)

add_executable(bundler src/main.cc)

target_link_libraries(bundler cxxopts termcolor stdc++fs cppglob)
target_include_directories(bundler PRIVATE deps/cppglob/include)

set_property(TARGET bundler PROPERTY CXX_STANDARD 17)
