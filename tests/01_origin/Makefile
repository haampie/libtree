# Basic test of $ORIGIN interpolation in rpath & runpath.

all: check

.PHONY: clean

liba.so: 
	echo 'int f(){return 1;}' | $(CC) -shared -Wl,-soname,$@ -o $@ -nostdlib -x c -

exe_rpath: liba.so
	echo 'int _start(){return f();}' | $(CC) -o $@ -Wl,--no-as-needed -Wl,--disable-new-dtags '-Wl,-rpath,$$ORIGIN' -Wno-implicit-function-declaration -nostdlib $< -x c -

exe_runpath: liba.so
	echo 'int _start(){return f();}' | $(CC) -o $@ -Wl,--no-as-needed -Wl,--enable-new-dtags '-Wl,-rpath,$$ORIGIN' -Wno-implicit-function-declaration -nostdlib $< -x c -

check: exe_rpath exe_runpath
	../../libtree exe_rpath
	../../libtree exe_runpath

clean:
	rm -f *.so exe*
