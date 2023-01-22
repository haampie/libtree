# Test 32 bits exectuables and libraries
# Make sure the 64 bits versions are not picked up.

LD_LIBRARY_PATH=

.PHONY: clean check

all: check

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

clean:
	rm -rf lib32 lib64 exe*

CURDIR ?= $(.CURDIR)

test-flag = 2>/dev/null ${CC} -E /dev/null
test-end = && echo y || echo n
support-m32 != ${test-flag} -m32 ${test-end}
support-m64 != ${test-flag} -m64 ${test-end}

check${support-m32:y=}:: exe32
	../../libtree exe32

check${support-m64:y=}:: exe64
	../../libtree exe64

check${support-m32:n=} check${support-m64:n=}::
	@echo WARNING: test skipped at ${CURDIR}
