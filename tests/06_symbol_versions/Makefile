# Creates two versions of a library with the same (so)name,
# and same symbol xyz, but versioned VER_V1 and VER_V2.
# As far as I understand, ld.so does not continue to look
# for another library in the search path which provides the
# correct symbol versions (e.g. link to VER_V2, put the VER_V1
# version first in the search ath, and glibc fixes this lib
# and simply errors).

LD_LIBRARY_PATH:=

.PHONY: clean

all: check

v1/libx.so: v1.c v1.map
	mkdir -p $(dir $@)
	$(CC) -shared -Wl,-soname,$(notdir $@) -o $@ -Wl,--version-script,$(word 2,$^) $<

v2/libx.so: v2.c v2.map
	mkdir -p $(dir $@)
	$(CC) -shared -Wl,-soname,$(notdir $@) -o $@ -Wl,--version-script,$(word 2,$^) $<

# this one is fine, link to v1, use v2 which provides the v1 symbol.
exe_v1: main.c v1/libx.so v2/libx.so
	$(CC) -o $@ $< $(word 2,$^) "-Wl,-rpath,$(CURDIR)/$(dir $(word 3,$^))" "-Wl,-rpath,$(CURDIR)/$(dir $(word 2,$^))"

# this one is not fine, link to v2, use v1 which does not provide the v2 symbol
exe_v2: main.c v1/libx.so v2/libx.so
	$(CC) -o $@ $< $(word 3,$^) "-Wl,-rpath,$(CURDIR)/$(dir $(word 2,$^))" "-Wl,-rpath,$(CURDIR)/$(dir $(word 3,$^))"

check: exe_v1 exe_v2
	../../libtree $(word 1,$^)
	../../libtree $(word 2,$^)

clean:
	rm -rf v1 v2 exe*

