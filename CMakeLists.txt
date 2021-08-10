cmake_minimum_required (VERSION 3.0)
project (libtree VERSION 2.0.0)

include(GNUInstallDirs)
include(CTest)

configure_file(libtree_version.hpp.in libtree_version.hpp)

option(LIBTREE_BUILD_TESTS "Build the tests" OFF)

# Find all dependencies
find_package(cxxopts CONFIG REQUIRED)
find_package(elfio CONFIG REQUIRED)
find_package(termcolor CONFIG REQUIRED)

include(CheckIncludeFiles)
check_include_files("sys/auxv.h" LIBTREE_HAS_AUXV_HEADER)

set(SOURCES
   ${CMAKE_CURRENT_SOURCE_DIR}/src/deploy.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/deps.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/elf.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/exec.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/glob.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/ld.cpp
)

# Add the library
add_library(libtree_lib OBJECT ${SOURCES})
set_target_properties(libtree_lib PROPERTIES OUTPUT_NAME libtree)
target_include_directories(libtree_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR})
target_link_libraries(libtree_lib termcolor::termcolor elfio::elfio)

# And the main cli tool
add_executable(libtree ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(libtree libtree_lib cxxopts::cxxopts)
target_compile_definitions(libtree PRIVATE $<$<BOOL:${LIBTREE_HAS_AUXV_HEADER}>:LIBTREE_HAS_AUXV_HEADER>)

set_property(TARGET libtree_lib PROPERTY CXX_STANDARD 17)
set_property(TARGET libtree PROPERTY CXX_STANDARD 17)

install(TARGETS libtree DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES doc/libtree.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

## Testing

if(LIBTREE_BUILD_TESTS)
    add_subdirectory(test)
endif()
