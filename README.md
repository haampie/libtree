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

- [Prebuilt binaries for **v3.1.0**](https://github.com/haampie/libtree/releases/tag/v3.1.0)
  | arch    | sha256sum |
  |---------|-----------|
  | [aarch64 (linux)](https://github.com/haampie/libtree/releases/download/v3.1.0/libtree_aarch64) | `c3d1e50801db8cd71427cccc00bb033211b68727ee5ff9858d5a22048fae4891` |
  | [armv6l (linux)](https://github.com/haampie/libtree/releases/download/v3.1.0/libtree_armv6l) | `4608828d01c383fdf5c02535bcce70493955a38e46c1ae0a1063c667625d11da` |
  | [armv7l (linux)](https://github.com/haampie/libtree/releases/download/v3.1.0/libtree_armv7l) | `9df3671b92151c01f37f3b38c1377dd2b179e559a033d9e23a22c669cc1cdae2` |
  | [i686 (linux)](https://github.com/haampie/libtree/releases/download/v3.1.0/libtree_i686) | `fb83c54196d77bee4ba83c42f014e5b8fc359d8b02114387f7ad46fedc83faf5` |
  | [x86_64 (linux)](https://github.com/haampie/libtree/releases/download/v3.1.0/libtree_x86_64) | `8d85183200300437b935574f259ed01efce4319eaec8525d87096c698a0f4c70` |
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
