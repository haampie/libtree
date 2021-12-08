libtree in C
- incomplete rewrite of `libtree`
- simpler to build (written in C, make as build system)
- faster (only reads the bare minimum from elf files)

todo:
- $LIB and $PLATFORM substitution in rpath.
- byte swapping for non-native endianness.
- bundling

don't-try-this-at-home install instructions:

```
curl -Lfs https://raw.githubusercontent.com/haampie/libtree-in-c/master/libtree.c | cc -o libtree -x c -
```

