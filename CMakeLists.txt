cmake_minimum_required (VERSION 3.0)
project (libtree VERSION 1.0.2)

include(GNUInstallDirs)

configure_file(libtree_version.hpp.in libtree_version.hpp)

option(USE_SYSTEM_DEPS "Use system cppglob, cxxopts, termcolor and elfio" OFF)

set(BUILD_SHARED OFF CACHE BOOL "Build shared version of deps")
set(BUILD_STATIC ON CACHE BOOL "Build static version of deps")

if(NOT USE_SYSTEM_DEPS)
    add_subdirectory(deps EXCLUDE_FROM_ALL)
endif()

set(SOURCE
   ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/deploy.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/deps.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/elf.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/exec.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/ld.cpp
)

add_executable(libtree ${SOURCE})
target_include_directories(libtree PRIVATE include ${PROJECT_BINARY_DIR})

if(USE_SYSTEM_DEPS)
    target_link_libraries(libtree cppglob)
else()
    target_link_libraries(libtree cxxopts termcolor ELFIO)

    # Todo, fix hardcoded SHARED/STATIC + separate targets
    # upstream to avoid the branch + variables here
    if (BUILD_SHARED)
        target_link_libraries(libtree cppglob)
    else()
        target_link_libraries(libtree cppglob_static)
    endif()
    
    target_include_directories(libtree PRIVATE
        deps/cppglob/include
        deps/termcolors/include
        deps/cxxopts/include/cxxopts
        deps/ELFIO
    )
endif()

set_property(TARGET libtree PROPERTY CXX_STANDARD 17)

install(TARGETS libtree
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
