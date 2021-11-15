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


# See new_root/CMakeLists.txt for details, this test is to ensure that we don't find
# libraries outside of the root directory.

include(ExternalProject)

ExternalProject_Add(
    new_root
    PREFIX new_root
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/new_root
    INSTALL_DIR
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}/new_root
)

# Without --root this should detect one - two - three.
add_test(NAME new_root_1
         COMMAND $<TARGET_FILE:libtree> ${CMAKE_CURRENT_BINARY_DIR}/new_root/three/two/one/one)

# root = <path to>/three, path = /two/one/one -- libthree is in reach.
add_test(NAME new_root_2
         COMMAND $<TARGET_FILE:libtree> --root ${CMAKE_CURRENT_BINARY_DIR}/new_root/three /two/one/one)
set_tests_properties(new_root_2 PROPERTIES PASS_REGULAR_EXPRESSION "libthree.so \\[r(un)?path\\]")

# root = <path to>/three/two, path = /one/one -- libthree is out of reach.
add_test(NAME new_root_3
         COMMAND $<TARGET_FILE:libtree> --root ${CMAKE_CURRENT_BINARY_DIR}/new_root/three/two /one/one)
set_tests_properties(new_root_3 PROPERTIES PASS_REGULAR_EXPRESSION "libthree.so not found")