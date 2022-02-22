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

- [Prebuilt binaries for **v3.0.3**](https://github.com/haampie/libtree/releases/tag/v3.0.3)
  | arch    | sha256sum |
  |---------|-----------|
  | [aarch64 (linux)](https://github.com/haampie/libtree/releases/download/v3.0.3/libtree_aarch64) | `6f5945b6fc2c7f82564f045054dbd04362567397f6ed6c816f161afd41067c21` |
  | [armv6l (linux)](https://github.com/haampie/libtree/releases/download/v3.0.3/libtree_armv6l) | `aed70facf4987f6e320bd88fbd0d5be2b7453e485ac3eb8e365011f588761bfd` |
  | [armv7l (linux)](https://github.com/haampie/libtree/releases/download/v3.0.3/libtree_armv7l) | `932284ce9897365f0623f4c802407aade63a8c6fd5cba523d20d77d683768fae` |
  | [i686 (linux)](https://github.com/haampie/libtree/releases/download/v3.0.3/libtree_i686) | `9e451ff5bd5dd229b65c48354d7136a54816da527308189ed045cea40a552c82` |
  | [x86_64 (linux)](https://github.com/haampie/libtree/releases/download/v3.0.3/libtree_x86_64) | `22ec893cc34892f88f25e42ba898314a480c7ab8456dcad2bdc1809e0e9d68b0` |
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
