# bundler

A tool to bundle your binaries, useful for building small Docker containers or AppImages.

Current functionality:
- [x] Add executables with `-e` and libraries via `-l`
- [x] Walks the dependency tree like `ld.so` (handles `RPATH`, `RUNPATH` and `LD_LIBRARY_PATH` correctly).
- [x] Uses `/etc/ld.so.conf` or any custom conf file via `-ldconf /path/to/ld.so.conf`
- [x] Skips blacklisted dependencies (and their dependencies) such as libc.so and libstdc++.so.
- [ ] Deploy binaries and rewrite their `RUNPATH`s.

```bash
git clone --recursive https://github.com/haampie/bundler.git
cd bundler
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

```bash
$ ./bundler -e bundler
bundler
 libcppglob.so.1
  libstdc++.so.6 (excluded)
  libgcc_s.so.1 (excluded)
  libc.so.6 (excluded)
 libstdc++.so.6 (excluded)
 libgcc_s.so.1 (excluded)
 libc.so.6 (excluded)
```
