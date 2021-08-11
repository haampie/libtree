# v2.0.0

- No changes to the libtree API
- Provide static executables for ease of use on distros with an old glibc or musl.
- Dropped the dependency on `cppglob`, use posix `glob` instead.
- No more vendored dependencies, rely on `find_package` to find
  `cxxopts`, `elfio`, and `termcolor`.