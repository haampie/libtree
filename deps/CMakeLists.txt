add_subdirectory(cppglob)

add_library(cxxopts INTERFACE)
target_include_directories(cxxopts INTERFACE cxxopts/include)

add_library(termcolor INTERFACE)
target_include_directories(termcolor INTERFACE termcolor/include)
