# Add test dependencies
add_subdirectory(deps)

# Add tests
add_executable(unit src/test.cpp)
target_link_libraries(unit libtree_lib gtest_main)
add_test(NAME unit COMMAND unit)
set_property(TARGET unit PROPERTY CXX_STANDARD 17)