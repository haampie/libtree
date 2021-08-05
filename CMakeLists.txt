cmake_minimum_required (VERSION 3.0)
project (libtree VERSION 1.2.2)

include(GNUInstallDirs)
include(CTest)

configure_file(libtree_version.hpp.in libtree_version.hpp)

option(USE_SYSTEM_DEPS "Use system cppglob, cxxopts, termcolor and elfio" OFF)

set(BUILD_SHARED OFF CACHE BOOL "Build shared version of deps")
set(BUILD_STATIC ON CACHE BOOL "Build static version of deps")

if(NOT USE_SYSTEM_DEPS)
    add_subdirectory(deps EXCLUDE_FROM_ALL)
endif()

include(CheckIncludeFiles)
check_include_files("sys/auxv.h" LIBTREE_HAS_AUXV_HEADER)

set(SOURCES
   ${CMAKE_CURRENT_SOURCE_DIR}/src/deploy.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/deps.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/elf.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/exec.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/ld.cpp
)

# Add the library
add_library(libtree_lib OBJECT ${SOURCES})
set_target_properties(libtree_lib PROPERTIES OUTPUT_NAME libtree)
target_include_directories(libtree_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR})

if(USE_SYSTEM_DEPS)
    target_link_libraries(libtree_lib cppglob)
else()
    target_link_libraries(libtree_lib cxxopts termcolor ELFIO)

    # Todo, fix hardcoded SHARED/STATIC + separate targets
    # upstream to avoid the branch + variables here
    if (BUILD_SHARED)
        target_link_libraries(libtree_lib cppglob)
    else()
        target_link_libraries(libtree_lib cppglob_static)
    endif()
    
    target_include_directories(libtree_lib PUBLIC
        deps/cppglob/include
        deps/termcolors/include
        deps/cxxopts/include/cxxopts
        deps/ELFIO
    )
endif()

# And the main cli tool
add_executable(libtree ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(libtree libtree_lib)
target_compile_definitions(libtree PRIVATE $<$<BOOL:${LIBTREE_HAS_AUXV_HEADER}>:LIBTREE_HAS_AUXV_HEADER>)

set_property(TARGET libtree_lib PROPERTY CXX_STANDARD 17)
set_property(TARGET libtree PROPERTY CXX_STANDARD 17)

install(TARGETS libtree DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES doc/libtree.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

## Testing

if(BUILD_TESTING)
    add_subdirectory(test)
endif()
