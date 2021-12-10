# libtree

A tool that:
- :deciduous_tree: turns `ldd` into a tree
- :point_up: explains how shared libraries are found or why they cannot be located

## Installation

`libtree` requires a C compiler that understands c99:

```
curl -Lfs https://raw.githubusercontent.com/haampie/libtree-in-c/master/libtree.c | cc -o libtree -x c -
```
