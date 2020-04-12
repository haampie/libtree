cmake_minimum_required (VERSION 3.0)
project (libtree)

set(CACHE BOOL USE_SYSTEM_DEPS OFF)

message(STATUS "Generating excludelist")
execute_process(
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/src/generate_exclude_list.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

if(NOT ${USE_SYSTEM_DEPS})
    add_subdirectory(deps)
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
target_include_directories(libtree PRIVATE
    include
    deps/cppglob/include
    deps/cxxopts/include
    ${CMAKE_CURRENT_BINARY_DIR}
)

if(${USE_SYSTEM_DEPS})
   target_link_libraries(libtree cppglob)
else()
   target_link_libraries(libtree cxxopts termcolor cppglob ELFIO)
endif()

set_property(TARGET libtree PROPERTY CXX_STANDARD 17)
