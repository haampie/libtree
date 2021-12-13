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
- [Upcoming version **v3.0.0-rc7**](https://github.com/haampie/libtree/releases/tag/v3.0.0-rc6)
  | arch    | sha256sum |
  |---------|-----------|
  | [aarch64 (linux)](https://github.com/haampie/libtree/releases/download/v3.0.0-rc7/libtree_aarch64) | `087226c69472a1cab61f38b78a157a6441e1ae859bae20c2d91977695e38b042` |
  | [armv6l (linux)](https://github.com/haampie/libtree/releases/download/v3.0.0-rc7/libtree_armv6l) | `40f2db85623178b91ae8eae8e91fc815191accd73abdd749cd0e55e2971a4864` |
  | [armv7l (linux)](https://github.com/haampie/libtree/releases/download/v3.0.0-rc7/libtree_armv7l) | `5be45efd474b8e117d62f3ae01ab5a0f7f40888000607a0b54b8e794d61da620` |
  | [i686 (linux)](https://github.com/haampie/libtree/releases/download/v3.0.0-rc7/libtree_i686) | `1031fbda8c0b3bc943fe32120eeeedb0d736247c285b8f0f69e61d85c264d5b9` |
  | [powerpc64le (linux)](https://github.com/haampie/libtree/releases/download/v3.0.0-rc7/libtree_powerpc64le) | `995abae57821f7529d72ee1650ae81422917ce933d7ecf0c35bc9d5f9d2dd761` | 
  | [x86_64 (linux)](https://github.com/haampie/libtree/releases/download/v3.0.0-rc7/libtree_x86_64) | `f8d36ef12a2895ee8f54a7c94f1f12f23406353b7f9e229f97e094e54be640b2` | 
  | [x86_64 (freebsd)](https://github.com/haampie/libtree/releases/download/v3.0.0-rc7/libtree_x86_64-freebsd) | `b4910a9a8a9e952670f66f5ab24e2a2bd8c22f2012089054082b596f675bfc79` | 

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
