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
- [Upcoming version **v3.0.0-rc9**](https://github.com/haampie/libtree/releases/tag/v3.0.0-rc9)
  | arch    | sha256sum |
  |---------|-----------|
  | [aarch64 (linux)](https://github.com/haampie/libtree/releases/download/v3.0.0-rc8/libtree_aarch64) | `30ba3ce5c8bfb163ee0bdd225fd777a21f067d1733b71eb4ae61c13a4b3d176a` |
  | [armv6l (linux)](https://github.com/haampie/libtree/releases/download/v3.0.0-rc9/libtree_armv6l) | `73ab358521a0b6c07fd6b8b76864666991876a7f74941bb059987d0dd2d7ff00` |
  | [armv7l (linux)](https://github.com/haampie/libtree/releases/download/v3.0.0-rc9/libtree_armv7l) | `7c9deb8c056f98a0892c15f30220b2f251d9283bfa3d3e2e3f669de23a7641a5` |
  | [i686 (linux)](https://github.com/haampie/libtree/releases/download/v3.0.0-rc9/libtree_i686) | `fddcecf94caa5ae9297b0ea6e1671e330433f121cd6befb9fce0886b4dad422e` |
  | [x86_64 (linux)](https://github.com/haampie/libtree/releases/download/v3.0.0-rc9/libtree_x86_64) | `48852e3111016551d12015b326e4dbc841c25539770196e2bb8b308a41fa27db` | 

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
curl -Lfs https://raw.githubusercontent.com/haampie/libtree/master/libtree.c | ${CC:-cc} -o libtree -x c - -std=c99 -D_FILE_OFFSET_BITS=64
```
</details>
