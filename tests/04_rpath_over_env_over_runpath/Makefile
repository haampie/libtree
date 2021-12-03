LD_LIBRARY_PATH:=

# test whether RUNPATH < LD_LIBRARY_PATH < RPATH
# the exe's both need libb.so, but there are two of those:
# ./dir/libb.so needs liba.so
# ./libb.so needs nothing.

.PHONY: clean

all: check

dir:
	mkdir $@

dir/liba.so: dir
	echo 'int a(){return 42;}' | $(CC) -shared -Wl,-soname,$(notdir $@) -Wl,--no-as-needed -o $@ -nostdlib -x c -

dir/libb.so: dir/liba.so
	echo 'int b(){return a();}' | $(CC) -shared -Wl,-soname,$(notdir $@) -Wl,--no-as-needed -Wl,--disable-new-dtags '-Wl,-rpath,$$ORIGIN' -Wno-implicit-function-declaration -o $@ -nostdlib $< -x c -

libb.so:
	echo 'int b(){return 10;}' | $(CC) -shared -Wl,-soname,$(notdir $@) -Wl,--no-as-needed -o $@ -Wno-implicit-function-declaration -nostdlib -x c -

exe_rpath: libb.so
	echo 'int _start(){return b();}' | $(CC) -Wl,--no-as-needed -Wl,--disable-new-dtags "-Wl,-rpath,$(CURDIR)" $< -o $@ -Wno-implicit-function-declaration -nostdlib -x c -

exe_runpath: libb.so
	echo 'int _start(){return b();}' | $(CC) -Wl,--no-as-needed -Wl,--enable-new-dtags "-Wl,-rpath,$(CURDIR)" $< -o $@ -Wno-implicit-function-declaration -nostdlib -x c -

check: exe_rpath exe_runpath dir/libb.so
	../../libtree exe_rpath
	LD_LIBRARY_PATH="$(CURDIR)/dir" ../../libtree exe_rpath
	../../libtree exe_runpath
	LD_LIBRARY_PATH="$(CURDIR)/dir" ../../libtree exe_runpath

clean:
	rm -rf *.so dir exe*

