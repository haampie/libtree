# Add test dependencies
add_subdirectory(deps)

# Add tests
add_executable(unit src/test.cpp)
target_link_libraries(unit libtree_lib gtest_main)
add_test(NAME unit COMMAND unit)
set_property(TARGET unit PROPERTY CXX_STANDARD 17)

# Integration test
find_program(BASH_PROGRAM bash)

add_custom_target(integration_test ALL COMMAND make WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/integration)

if(BASH_PROGRAM)
    add_test(NAME integration_test COMMAND ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/integration/run_test.sh $<TARGET_FILE:libtree>)
endif()
