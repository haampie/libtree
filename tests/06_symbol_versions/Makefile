# Creates two versions of a library with the same (so)name,
# and same symbol xyz, but versioned VER_V1 and VER_V2.
# As far as I understand, ld.so does not continue to look
# for another library in the search path which provides the
# correct symbol versions (e.g. link to VER_V2, put the VER_V1
# version first in the search ath, and glibc fixes this lib
# and simply errors).

LD_LIBRARY_PATH=

.PHONY: clean

all: check

v1/libx.so: v1.c v1.map
v2/libx.so: v2.c v2.map
v1/libx.so v2/libx.so:
	mkdir -p $(@D)
	$(CC) -shared -Wl,-soname,$(@F) -o $@ -Wl,--version-script,$(@D).map $(@D).c

# this one is fine, link to v1, use v2 which provides the v1 symbol.
exe_v1: main.c v1/libx.so v2/libx.so
	$(CC) -o $@ main.c v1/libx.so "-Wl,-rpath,$(CURDIR)/v2" "-Wl,-rpath,$(CURDIR)/v1"

# this one is not fine, link to v2, use v1 which does not provide the v2 symbol
exe_v2: main.c v1/libx.so v2/libx.so
	$(CC) -o $@ main.c v2/libx.so "-Wl,-rpath,$(CURDIR)/v1" "-Wl,-rpath,$(CURDIR)/v2"

check: exe_v1 exe_v2
	../../libtree exe_v1
	../../libtree exe_v2

clean:
	rm -rf v1 v2 exe*

CURDIR ?= $(.CURDIR)
