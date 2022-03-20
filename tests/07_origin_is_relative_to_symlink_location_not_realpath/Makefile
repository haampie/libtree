# ORIGIN should be resolved relative to the directory the lib was found in,
# even if what we've found is a symlink.  So this test creates exe <- a/libf <-
# a/libg and a symlink at b/libf to a/libf, where a/libf has ${ORIGIN} rpath.

LD_LIBRARY_PATH=

.PHONY: clean

all: check

a/libf.so: 
	mkdir -p $(@D)
	echo 'int f(){return 3;};' | $(CC) -shared -Wl,-soname,$(@F) -o $@ -x c - -nostdlib

a/libg.so: a/libf.so
	mkdir -p $(@D)
	echo 'extern int f(); int g(){return f();};' | $(CC) -shared -Wl,-soname,$(@F) '-Wl,-rpath,$${ORIGIN}' -o $@ -x c - -La -lf -nostdlib

b/libg.so: a/libg.so
	mkdir -p $(@D)
	ln -vs ../a/libg.so $@

exe: a/libg.so b/libg.so
	echo 'extern int g(); int _start(){return g();};' | $(CC) -Wl,-soname,$(@F) '-Wl,-rpath,$${ORIGIN}/b' -o $@ -x c - -La -lg -nostdlib

check: exe
	! ../../libtree exe # should not find libf.so
	LD_LIBRARY_PATH=$(CURDIR)/a ../../libtree exe # should find libf.so

clean:
	rm -rf a b exe*

CURDIR ?= $(.CURDIR)
