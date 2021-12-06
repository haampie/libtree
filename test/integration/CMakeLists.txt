cmake_minimum_required (VERSION 3.0)
project (integration_test)

file(COPY ${CMAKE_CURRENT_LIST_DIR}/run_test.sh
          ${CMAKE_CURRENT_LIST_DIR}/test_1_expected.txt
          ${CMAKE_CURRENT_LIST_DIR}/test_2_expected.txt
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(src)
