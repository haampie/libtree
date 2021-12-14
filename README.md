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

- [Latest version **v3.0.0**](https://github.com/haampie/libtree/releases/tag/v3.0.0)
  | arch    | sha256sum |
  |---------|-----------|
  | [aarch64 (linux)](https://github.com/haampie/libtree/releases/download/v3.0.0/libtree_aarch64) | `50262cce5523ed0e19e38d563e9eacf69aa4921e01cd9735d96d05d0c1fc3018` |
  | [armv6l (linux)](https://github.com/haampie/libtree/releases/download/v3.0.0/libtree_armv6l) | `2341ff61152056b3f72776983d27ee8620c61e5f5476454a40ab909cfbcbcc85` |
  | [armv7l (linux)](https://github.com/haampie/libtree/releases/download/v3.0.0/libtree_armv7l) | `f3801b197a3ea192697a13d62cb653d5603a68f34cd9350d8ea717184614dde0` |
  | [i686 (linux)](https://github.com/haampie/libtree/releases/download/v3.0.0/libtree_i686) | `28c080c70715a3c1b5354e8de04f6edeb3fd9e6e840c9de7bd28ef2f23b3b16b` |
  | [x86_64 (linux)](https://github.com/haampie/libtree/releases/download/v3.0.0/libtree_x86_64) | `6acee10fd0a9bf18f71f5cb9cc1a41309e063d7eb682f8425355d1346f8c5a13` | 
- [Older release **v2.0.0**](https://github.com/haampie/libtree/releases/tag/v2.0.0)


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
