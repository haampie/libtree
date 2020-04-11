# libtree

A tool that:
- :deciduous_tree: turns `ldd` into a fancy tree
- :point_up: explains why `ldd` finds shared libraries and why not
- :package: optionally deploys executables and dependencies into a single directory

## Installation
Download the [**latest release**](https://github.com/haampie/libtree/releases) from Github:

```
- wget https://github.com/haampie/libtree/releases/download/v1.0.0/libtree_x86_64.tar.gz
- tar -xzf libtree_x86_64.tar.gz
- ./libtree/libtree -e $(which man)
```

## Example 1: listing the dependencies of an executable

![example](doc/screenshot.png)

## Example 2: deploying binaries + dependencies into a folder:
```bash
$ libtree -e $(which libtree) -d libtree.bundle
Dependency tree
libtree
├── libcppglob.so.1 [runpath]
│   ├── libstdc++.so.6 (skipped) [ld.so.conf]
│   ├── libgcc_s.so.1 (skipped) [ld.so.conf]
│   └── libc.so.6 (skipped) [ld.so.conf]
├── libstdc++.so.6 (skipped) [ld.so.conf]
├── libgcc_s.so.1 (skipped) [ld.so.conf]
└── libc.so.6 (skipped) [ld.so.conf]

Deploying to "libtree.bundle/usr"
"/home/.../Documents/projects/libtree/build/libtree" => "libtree.bundle/usr/bin/libtree"
"/home/.../Documents/projects/libtree/build/lib/libcppglob.so.1.1.0" => "libtree.bundle/usr/lib/libcppglob.so.1.1.0"
  creating symlink "libtree.bundle/usr/lib/libcppglob.so.1"

$ tree libtree.bundle/
libtree.bundle/
└── usr
    ├── bin
    │   └── libtree
    └── lib
        ├── libcppglob.so.1 -> libcppglob.so.1.1.0
        └── libcppglob.so.1.1.0

3 directories, 3 files
```

## Build from source 
```bash
git clone --recursive https://github.com/haampie/libtree.git
cd libtree
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

## Current functionality
- [x] Add executables with `-e` and libraries via `-l`
- [x] Walks the dependency tree like `ld.so` (handles `RPATH`, `RUNPATH` and `LD_LIBRARY_PATH` correctly).
- [x] Uses `/etc/ld.so.conf` or any custom conf file via `-ldconf /path/to/ld.so.conf`
- [x] Skips blacklisted dependencies (and their dependencies) such as libc.so and libstdc++.so.
- [x] Deploy binaries and rewrite their `RUNPATH`s\*
- [x] Ship `chrpath` and `strip`
- [ ] i386 (currently it's hardcoded to only deploy x86_64)

\* Note: `patchelf` seems to be very broken software, so instead I'm using `chrpath`. The downside is `chrpath` will only patch rpaths _when they already exist in the binary_. Therefore you might still need to add the `yourapp/lib` folder to ld's search paths by running `echo /path/to/yourapp.bundle/lib > /etc/ld.so.conf/my_app.conf && ldconfig` or by setting `LD_LIBRARY_PATH=/path/to/yourapp.bundle/lib`.
