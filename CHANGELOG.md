# v2.1.0
- New `--root path/to/rootfs` flag, which is useful when doing cross-compilation with a root filesystem for the target architecture.

# v2.0.0

- No changes to the libtree API
- Provide static executables for ease of use on distros with an old glibc or musl.
- Dropped the dependency on `cppglob`, use posix `glob` instead.
- No more vendored dependencies, rely on `find_package` to find
  `cxxopts`, `elfio`, and `termcolor`.
