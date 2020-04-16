![Test](https://github.com/haampie/libtree/workflows/Test/badge.svg?branch=master)

# libtree

A tool that:
- :deciduous_tree: turns `ldd` into a tree
- :point_up: explains why shared libraries are found and why not
- :package: optionally deploys executables and dependencies into a single directory

## Installation
Download the [**latest release**](https://github.com/haampie/libtree/releases) from Github:

```bash
wget https://github.com/haampie/libtree/releases/download/v1.1.2/libtree_x86_64.tar.gz
tar -xzf libtree_x86_64.tar.gz
ln -s $(realpath ./libtree/libtree) /usr/local/bin/
libtree $(which man)
```

## Example 1: listing the dependencies of an executable

![example](doc/screenshot.png)

## Example 2: deploying binaries + dependencies into a folder
```bash
$ libtree $(which libtree) -d libtree.bundle --chrpath --strip
libtree
└── libcppglob.so.1 [runpath]

Deploying to "libtree.bundle/usr"
"/bundler/build/libtree" => "libtree.bundle/usr/bin/libtree"
"/bundler/build/lib/libcppglob.so.1.1.0" => "libtree.bundle/usr/lib/libcppglob.so.1.1.0"
  creating symlink "libtree.bundle/usr/lib/libcppglob.so.1"

$ tree libtree.bundle/
libtree.bundle/
└── usr
    ├── bin
    │   └── libtree
    └── lib
        ├── libcppglob.so.1 -> libcppglob.so.1.1.0
        └── libcppglob.so.1.1.0

3 directories, 3 files
```

## Verbose output
By default certain standard depenendencies are not shown. For more verbose output use
-  `libtree -v $(which man)` to show skipped libraries without their children
-  `libtree -a $(which apt-get)` to show the full recursive list of libraries

Use the `--path` or `-p` flags to show paths rather than soname's:

- `libtree -p $(which tar)`

## Changing search paths
`libtree` follows the rules of `ld.so` to locate libraries, but does not use `ldconfig`'s
cache. Instead it parses `/etc/ld.so.conf` at runtime. In fact you can change the search
path config by setting `--ldconf mylibs.conf`. Search paths can be added as well via 
`LD_LIBRARY_PATH="path1:path2:$LD_LIBRARY_PATH" libtree ...`.

## Build from source 
```bash
git clone --recursive https://github.com/haampie/libtree.git
cd libtree
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
make install
```

## Known issues
- When deploying libs with `libtree app -d folder.bundle --chrpath`, the runpaths are only
  changed when the binaries already have an an rpath or runpath. This is a limitation of
  `chrpath`. Another option is to use `patchelf` instead, but this tool is known to break
  binaries sometimes.
