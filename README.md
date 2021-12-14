# libtree

A tool that:
- :deciduous_tree: turns `ldd` into a tree
- :point_up: explains how shared libraries are found or why they cannot be located

![Screenshot of libtree](doc/screenshot.png)


## Output

By default, certain standard dependencies are not shown. For more verbose output use

-  `libtree -v`             Show libraries skipped by default
-  `libtree -vv`            Show dependencies of libraries skipped by default
-  `libtree -vvv`           Show dependencies of already encountered libraries

Use the `--path` or `-p` flags to show paths rather than sonames:

- `libtree -p $(which tar)`


## Install

- [Current stable version **v2.0.0**](https://github.com/haampie/libtree/releases/tag/v2.0.0)
- [Upcoming version **v3.0.0-rc8**](https://github.com/haampie/libtree/releases/tag/v3.0.0-rc6)
  | arch    | sha256sum |
  |---------|-----------|
  | [aarch64 (linux)](https://github.com/haampie/libtree/releases/download/v3.0.0-rc8/libtree_aarch64) | `adf18be6f46da010c5a2c1eadda9341df9d7d63dedd2d005b494b16167e86167` |
  | [armv6l (linux)](https://github.com/haampie/libtree/releases/download/v3.0.0-rc8/libtree_armv6l) | `782ae0a8bc5bea27356525c76d3ac6b4e65677c2d6d36616486e8cf3bb5a5912` |
  | [armv7l (linux)](https://github.com/haampie/libtree/releases/download/v3.0.0-rc8/libtree_armv7l) | `8d3b91985af1babe080d53f8e74cefaf47ff6f58e29abd3ea8f03c7680bf0127` |
  | [i686 (linux)](https://github.com/haampie/libtree/releases/download/v3.0.0-rc8/libtree_i686) | `b54aaa59f2c54df08444ab09c61acd10f22ca4f6dcb29de775a0b498958225a0` |
  | [x86_64 (linux)](https://github.com/haampie/libtree/releases/download/v3.0.0-rc8/libtree_x86_64) | `4524da05b0921f7315ef3b2380e14567b5177a0c9dcbe0aaf889eab834ccf439` | 

## Building from sources

`libtree` requires a C compiler that understands c99

```
git clone https://github.com/haampie/libtree.git
cd libtree
make # recommended: LDFLAGS=-static
```

<details>
<summary>Or use the following unsafe quick install instructions</summary>

```
curl -Lfs https://raw.githubusercontent.com/haampie/libtree-in-c/master/libtree.c | ${CC:-cc} -o libtree -x c - -std=c99 -D_FILE_OFFSET_BITS=64
```
</details>
