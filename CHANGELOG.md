# v3.1.1
- Build system portability fixes
- Fix `make check` exit code

# v3.1.0
- Add a `--max-depth` flag to limit recursion depth. For example,
  `libtree --max-depth 1 <file>` will show the resolved paths of direct dependencies
  only.

# v3.0.2
- Improve `make check`, `make clean` and CI
- Preserve original timestamps when installing files
- Add rpath order test

# v3.0.1
- Fix man pages directory in `make install`
- Skip dynamic linker on aarch64 and powerpc

# v3.0.0
- Rewritten in C99 with 0 external dependencies.
- Significantly faster & smaller (~50KB statically compiled with musl libc, or
  even smaller than the source file with diet libc).
- Cross-compiled binaries now available thanks to
  [binarybuilder.org](https://binarybuilder.org/)
- Improved search path printing when libraries cannot be located.
- Improved rpath search: shows `[rpath of ...]` when lib is located by parent
  of parent ... of parent's rpath.
- `fd` inspired highlight of filename when printing paths.
- Caches files by inode instead of soname, which is useful in the sense that
  this allows you to find broken libraries that only work because of a
  particular search order of the tree. (Consider an executable A and libraries
  B, C and D, where A depends on B and C, and B and C depend on D:
  
  ```
    B
   / \
  A   D
   \ /
    C
  ```

  It may happen that D *can* be located through B's rpath, but not through C's.
  Then, depending on whether A - B - D is traversed first, or A - C - D, ld.so
  will complain about missing libraries or not. `libtree` on the other hand
  will always tell you that D can't be located through C.
- More verbosity levels `-v`, `-vv`, `-vvv` instead of `-a` and `-v` flags.
- Skip fewer libraries by default (only libc / libstdc++ type of libs).
- `PLATFORM` rpath interpolation now uses `uname`, this is not always the same
  as `AT_PLATFORM`, but unlikely to be different, and in fact the feature is
  rarely used.
- Support for `NODEFLIB` flag, which is a dynamic array entry flag that signals
  to the dynamic linker that it should not search default system paths
  including those specified in `ld.so.conf`.
- Better FreeBSD support (`OSREL`, `OSNAME` interpolation in rpaths and
  `/etc/ld-elf.so.conf` config file support)
- Support for relative includes in `ld.so.conf` config files.

Breaking changes:
- The bundling feature was dropped in `3.0.0`, but is still supported in `2.x`.
  It may return in a future `3.x` release, but my impression is that there are
  excellent tools like Exodus which do a better job at bundling (in particular:
  they ship a copy of the dynamic linker.)
- The `--skip` and `--platform` flags were removed.

# v2.0.0

- No changes to the libtree API
- Provide static executables for ease of use on distros with an old glibc or
  musl.
- Dropped the dependency on `cppglob`, use posix `glob` instead.
- No more vendored dependencies, rely on `find_package` to find `cxxopts`,
  `elfio`, and `termcolor`.
