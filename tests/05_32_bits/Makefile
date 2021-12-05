# Test 32 bits exectuables and libraries
# Make sure the 64 bits versions are not picked up.

LD_LIBRARY_PATH:=

.PHONY: clean

all: check

lib64/libx.so:
	mkdir -p $(dir $@)
	echo 'int a(){return 42;}' | $(CC) -shared -Wl,-soname,$(notdir $@) -m64 -o $@ -nostdlib -x c -

lib32/libx.so:
	mkdir -p $(dir $@)
	echo 'int a(){return 42;}' | $(CC) -shared -Wl,-soname,$(notdir $@) -m32 -o $@ -nostdlib -x c -

exe64: lib64/libx.so
	echo 'extern int a(); int _start(){return a();}' | $(CC) -m64 "-Wl,-rpath,$(CURDIR)/lib32" "-Wl,-rpath,$(CURDIR)/lib64" -o $@ -nostdlib -x c - -Llib64 -lx

exe32: lib32/libx.so
	echo 'extern int a(); int _start(){return a();}' | $(CC) -m32 "-Wl,-rpath,$(CURDIR)/lib64" "-Wl,-rpath,$(CURDIR)/lib32" -o $@ -nostdlib -x c - -Llib32 -lx

check: exe32 exe64
	../../libtree exe32
	../../libtree exe64

clean:
	rm -rf lib32 lib64 exe*

