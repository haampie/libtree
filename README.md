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

- [Latest version **v3.0.1**](https://github.com/haampie/libtree/releases/tag/v3.0.2)
  | arch    | sha256sum |
  |---------|-----------|
  | [aarch64 (linux)](https://github.com/haampie/libtree/releases/download/v3.0.2/libtree_aarch64) | `7c101b43bec7d0caa2370b9bf3e827a9366b3c17a030fd80c4353f8f58111f17` |
  | [armv6l (linux)](https://github.com/haampie/libtree/releases/download/v3.0.2/libtree_armv6l) | `c5a844cd128a588c26004601881e381cf3292676bfba63deba00e0b3e46da693` |
  | [armv7l (linux)](https://github.com/haampie/libtree/releases/download/v3.0.2/libtree_armv7l) | `cebfef92f5cfdfacbb878982a0d444d6b2cf9479b90e9aede4165be72d1692b2` |
  | [i686 (linux)](https://github.com/haampie/libtree/releases/download/v3.0.2/libtree_i686) | `bb314da7a27e57424af55218506e8f48359176fa994db4fea00b9794e4083ac8` |
  | [x86_64 (linux)](https://github.com/haampie/libtree/releases/download/v3.0.2/libtree_x86_64) | `9f29b7f0a737034114bb13c8779b743d838616d517cd8a2bae54565e9b4d1f7e` | 
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
