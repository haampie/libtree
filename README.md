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

Use `--max-depth` to limit the recursion depth.


## Install

- [Prebuilt binaries for **v3.1.1**](https://github.com/haampie/libtree/releases/tag/v3.1.1)
  | arch    | sha256sum |
  |---------|-----------|
  | [aarch64 (linux)](https://github.com/haampie/libtree/releases/download/v3.1.1/libtree_aarch64) | `c5d4fbcd4e3fb46f02c028532f60fcf1c92f7c6aad5b07a991c67550c2554862` |
  | [armv6l (linux)](https://github.com/haampie/libtree/releases/download/v3.1.1/libtree_armv6l) | `16f5a7503a095bd88ebc5e21ec4ba8337c5d9712cac355bf89399c9e6beef661` |
  | [armv7l (linux)](https://github.com/haampie/libtree/releases/download/v3.1.1/libtree_armv7l) | `17f493621e7cc651e2bddef207c1554a64a114e1c907dbe5b79ff0e97180b29e` |
  | [i686 (linux)](https://github.com/haampie/libtree/releases/download/v3.1.1/libtree_i686) | `230a163c20f4a88a983d8647a9aa793317be6556e2c6a79e8a6295389e651ef5` |
  | [x86_64 (linux)](https://github.com/haampie/libtree/releases/download/v3.1.1/libtree_x86_64) | `49218482f89648972ea4ef38cf986e85268efd1ce8f27fe14b23124bca009e6f` |
- Fedora / RHEL / CentOS
  ```console
  $ dnf install epel-release # For RHEL and derivatives enable EPEL first 
  $ dnf install libtree-ldd
  ```
- Ubuntu 22.04+
  ```console
  apt-get install libtree
  ```
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
