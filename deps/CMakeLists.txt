add_subdirectory(cppglob)

add_library(cxxopts INTERFACE)
target_include_directories(cxxopts INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/cxxopts/include)

add_library(termcolor INTERFACE)
target_include_directories(termcolor INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/termcolor/include)

add_library(ELFIO INTERFACE)
target_include_directories(ELFIO INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/ELFIO/)
