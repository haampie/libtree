# bundler

A tool that:
- :deciduous_tree: turns `ldd` into a fancy tree
- :point_up: explains why `ldd` finds shared libraries and why not
- :package: optionally deploys relevant executables and dependencies into a single directory

## Installation
Download the [**latest release**](https://github.com/haampie/bundler/releases) from Github:

```
- wget https://github.com/haampie/bundler/releases/download/v0.1.4/bundler_x86_64.tar.gz
- tar -xzf bundler_x86_64.tar.gz
- ./bundler/bundler -e $(which man)

## Example 1: listing the dependencies of an executable

![example](doc/screenshot.png)

## Example 2: deploying binaries + dependencies into a folder:
```bash
$ bundler -e $(which bundler) -d bundler.bundle
Dependency tree
bundler
├── libcppglob.so.1 [direct]
│   ├── libstdc++.so.6 (skipped) [ld.so.conf]
│   ├── libgcc_s.so.1 (skipped) [ld.so.conf]
│   └── libc.so.6 (skipped) [ld.so.conf]
├── libstdc++.so.6 (skipped) [ld.so.conf]
├── libgcc_s.so.1 (skipped) [ld.so.conf]
└── libc.so.6 (skipped) [ld.so.conf]

Deploying to "bundler.bundle/usr"
"/home/.../Documents/projects/bundler/build/bundler" => "bundler.bundle/usr/bin/bundler"
"/home/.../Documents/projects/bundler/build/lib/libcppglob.so.1.1.0" => "bundler.bundle/usr/lib/libcppglob.so.1.1.0"
  creating symlink "bundler.bundle/usr/lib/libcppglob.so.1"

$ tree bundler.bundle/
bundler.bundle/
└── usr
    ├── bin
    │   └── bundler
    └── lib
        ├── libcppglob.so.1 -> libcppglob.so.1.1.0
        └── libcppglob.so.1.1.0

3 directories, 3 files
```

## Build from source 
```bash
git clone --recursive https://github.com/haampie/bundler.git
cd bundler
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
