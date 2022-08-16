# Test 32 bits exectuables and libraries
# Make sure the 64 bits versions are not picked up.

LD_LIBRARY_PATH=
CPU_ARCH=$(shell uname -m)

.PHONY: clean

all: check

ifeq ($(CPU_ARCH), $(filter x86_64 amd64 i686 i386 ppc% sparc64,$(CPU_ARCH)))
lib64/libx.so:
	mkdir -p $(@D)
	echo 'int a(){return 42;}' | $(CC) -shared -Wl,-soname,$(@F) -m64 -o $@ -nostdlib -x c -

lib32/libx.so:
	mkdir -p $(@D)
	echo 'int a(){return 42;}' | $(CC) -shared -Wl,-soname,$(@F) -m32 -o $@ -nostdlib -x c -

exe64: lib64/libx.so
	echo 'extern int a(); int _start(){return a();}' | $(CC) -m64 "-Wl,-rpath,$(CURDIR)/lib32" "-Wl,-rpath,$(CURDIR)/lib64" -o $@ -nostdlib -x c - -Llib64 -lx

exe32: lib32/libx.so
	echo 'extern int a(); int _start(){return a();}' | $(CC) -m32 "-Wl,-rpath,$(CURDIR)/lib64" "-Wl,-rpath,$(CURDIR)/lib32" -o $@ -nostdlib -x c - -Llib32 -lx

check: exe32 exe64
	../../libtree exe32
	../../libtree exe64

else
check:
	echo Architecture does not support -m32 or -m64 options. Do nothing.
endif

clean:
	rm -rf lib32 lib64 exe*

CURDIR ?= $(.CURDIR)
