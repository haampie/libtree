# bundler

Bundle executables and libraries to a single folder.

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
