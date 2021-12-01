libtree in C

- incomplete rewrite of `libtree`
- simpler to build (written in C, make as build system)
- faster (only reads the bare minimum from elf files)

todo:
- ld.so.conf parsing
- rpath substitution
- byte swapping for non-native endianness
- 32 bit
- bundling

