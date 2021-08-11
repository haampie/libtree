# Use CMake 3.20's GTest::gtest target.
cmake_minimum_required(VERSION 3.20)

# Test dependencies
find_package(GTest REQUIRED)
find_program(BASH_PROGRAM bash REQUIRED)

add_executable(deps src/deps.cpp)
target_link_libraries(deps libtree_lib GTest::gtest_main)
set_property(TARGET deps PROPERTY CXX_STANDARD 17)
add_test(NAME deps COMMAND deps)

add_executable(ld_so_conf src/ld_so_conf.cpp)
target_link_libraries(ld_so_conf libtree_lib GTest::gtest)
set_property(TARGET ld_so_conf PROPERTY CXX_STANDARD 17)
add_test(NAME ld_so_conf COMMAND ld_so_conf ${CMAKE_CURRENT_SOURCE_DIR}/ld_so_conf/ld.so.conf)

# Integration tests
add_subdirectory(integration)

if(BASH_PROGRAM)
    add_test(NAME integration_test COMMAND ${BASH_PROGRAM} ${CMAKE_CURRENT_BINARY_DIR}/integration/run_test.sh $<TARGET_FILE:libtree>)
endif()
