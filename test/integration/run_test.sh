#!/bin/bash

cd "$(dirname "$0")"

set -e

if [ $# -ne 1 ]; then
    echo "Usage: $0 /path/to/libtee"
    exit 1
fi

# Test without setting LD_LIBRARY_PATH
TEST_1=`$1 src/main`
EXPECTED_1=$(cat test_1_out.txt)

if [ "$TEST_1" != "$EXPECTED_1" ]; then
    diff --color <( echo "$TEST_1" ) <( echo "$EXPECTED_1")
    exit 1
fi

# Test with LD_LIBRARY_PATH
TEST_2=`LD_LIBRARY_PATH=$PWD/src/a/b/c $1 src/main`
EXPECTED_2=$(cat test_2_out.txt)

if [ "$TEST_2" != "$EXPECTED_2" ]; then
    diff --color <( echo "$TEST_2" ) <( echo "$EXPECTED_2")
    exit 1
fi
