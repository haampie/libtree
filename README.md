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

- [Latest version **v3.0.1**](https://github.com/haampie/libtree/releases/tag/v3.0.1)
  | arch    | sha256sum |
  |---------|-----------|
  | [aarch64 (linux)](https://github.com/haampie/libtree/releases/download/v3.0.1/libtree_aarch64) | `671a7ab57a068d5106595680389dd17c632c5eeb0b54bef4e3d22e1565f5676e` |
  | [armv6l (linux)](https://github.com/haampie/libtree/releases/download/v3.0.1/libtree_armv6l) | `a28c1c2bbd3e3d28c8788fff5ff8494e2131701fbdec2aab59b9154b9e90eb7f` |
  | [armv7l (linux)](https://github.com/haampie/libtree/releases/download/v3.0.1/libtree_armv7l) | `c1fa914db49f1b794579a95a483c09fb3aea5a9630cb8535af03b9d292cda379` |
  | [i686 (linux)](https://github.com/haampie/libtree/releases/download/v3.0.1/libtree_i686) | `57009c08c50fe290dff135e8444972fe6b86a2839fb8bd6d79975829a7a0a257` |
  | [x86_64 (linux)](https://github.com/haampie/libtree/releases/download/v3.0.1/libtree_x86_64) | `64b573b5cfadd584c2467bfa50614c78100a83088698ba96b034b17347f4bb28` | 
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
