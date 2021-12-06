libtree in C
- incomplete rewrite of `libtree`
- simpler to build (written in C, make as build system)
- faster (only reads the bare minimum from elf files)

todo:
- arg parsing
- $LIB and $PLATFORM substitution in rpath.
- byte swapping for non-native endianness.
- bundling

