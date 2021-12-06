#!/bin/sh

cd "$(dirname "$0")"

set -e

if [ $# -ne 1 ]; then
    echo "Usage: $0 /path/to/libtee"
    exit 1
fi

if [ "$(uname -s)" = "Linux" ]; then
    DIFF="diff --color"
else
    DIFF="diff"
fi

# Test without setting LD_LIBRARY_PATH
"$1" src/main > test_1_out.txt

if ! cmp --silent -- test_1_out.txt test_1_expected.txt; then
    $DIFF test_1_out.txt test_1_expected.txt
    exit 1
fi

# Test with LD_LIBRARY_PATH
LD_LIBRARY_PATH="$PWD/src/a/b/c" "$1" src/main > test_2_out.txt

if ! cmp --silent -- test_2_out.txt test_2_expected.txt; then
    $DIFF test_2_out.txt test_2_expected.txt
    exit 1
fi
