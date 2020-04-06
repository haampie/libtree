# bundler

A tool to bundle your binaries, useful for building small Docker containers or AppImages.

Current functionality:
- [x] Add executables with `-e` and libraries via `-l`
- [x] Walks the dependency tree like `ld.so` (handles `RPATH`, `RUNPATH` and `LD_LIBRARY_PATH` correctly).
- [x] Uses `/etc/ld.so.conf` or any custom conf file via `-ldconf /path/to/ld.so.conf`
- [x] Skips blacklisted dependencies (and their dependencies) such as libc.so and libstdc++.so.
- [x] Deploy binaries and rewrite their `RUNPATH`s\*
- [ ] i386 (currently it's hardcoded to only deploy x86_64)

\* Note: `patchelf` seems to be very broken software, so instead I'm using `chrpath`, but this will only patch rpaths _when they already exist in the binary_. Therefore you might still need to add the `yourapp/lib` folder to ld's search paths by running `ldconfig yourapp/lib` or setting `LD_LIBRARY_PATH=yourapp/lib`.

```bash
git clone --recursive https://github.com/haampie/bundler.git
cd bundler
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

```bash
$ ./bundler -e bundler -d Bundler.app
Dependency tree
bundler
|--libcppglob.so.1
|  |--libstdc++.so.6 (excluded)
|  |--libgcc_s.so.1 (excluded)
|  |--libc.so.6 (excluded)
|--libstdc++.so.6 (excluded)
|--libgcc_s.so.1 (excluded)
|--libc.so.6 (excluded)

Deploying to "Bundler.app/usr"
"/home/harmen/Documents/projects/binary_bundler/build/bundler" => "Bundler.app/usr/bin/bundler"
"/home/harmen/Documents/projects/binary_bundler/build/lib/libcppglob.so.1.1.0" => "Bundler.app/usr/lib/libcppglob.so.1"

$ tree Bundler
Bundler.app/
└── usr
    ├── bin
    │   └── bundler
    └── lib
        └── libcppglob.so.1

3 directories, 2 files
```
