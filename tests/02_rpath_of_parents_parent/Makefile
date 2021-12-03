# This creates exe <- liba.so <- libb.so
# where liba.so cannot directly locate libb.so, but it *can* through exe's rpaths.

LD_LIBRARY_PATH:=

.PHONY: clean

all: check

libb.so: 
	echo 'int g(){return 1;}' | $(CC) -shared -Wl,-soname,$@ -o $@ -nostdlib -x c -

liba.so: libb.so
	echo 'int f(){return g();}' | $(CC) -shared -Wl,--no-as-needed -Wl,-soname,$@ -o $@ -Wno-implicit-function-declaration $< -nostdlib -x c -

exe: liba.so
	echo 'int _start(){return f();}' | $(CC) -o $@ -Wl,--no-as-needed -Wl,--disable-new-dtags '-Wl,-rpath,$$ORIGIN' '-Wl,-rpath-link,$(CURDIR)' -Wno-implicit-function-declaration -nostdlib -L. -la -x c -

check: exe liba.so
	../../libtree liba.so  # should not find libb.so
	LD_LIBRARY_PATH=$(CURDIR) ../../libtree liba.so  # should find libb.so through LD_LIBRARY_PATH
	../../libtree exe  # should find libb.so through exe's rpath

clean:
	rm -f *.so exe*
