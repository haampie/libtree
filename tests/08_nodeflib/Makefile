# When passing -z nodeflib, the runtime linker should not consider ld.so.conf
# nor standard paths (/usr/lib, /lib, ...). This is a little bit hard to test
# but in general it means that if you use the system compiler with this flag,
# libc itself would not be located.

.PHONY: clean

LD_LIBRARY_PATH:=

all: check

lib_nodefaultlib.so: 
	echo 'int f(){return 1;}' | $(CC) -z nodefaultlib -Wl,--no-as-needed -shared -Wl,-soname,$@ -o $@ -x c -

lib_defaultlib.so:
	echo 'int g(){return 1;}' | $(CC) -Wl,--no-as-needed -shared -Wl,-soname,$@ -o $@ -x c -

exe_nodefaultlib: lib_nodefaultlib.so lib_defaultlib.so
	echo 'extern int f(); extern int g(); int main(){return f() + g();}' | $(CC) -z nodefaultlib -o $@ -Wl,--enable-new-dtags '-Wl,-rpath,$$ORIGIN' -x c - -L. -l_nodefaultlib -l_defaultlib

check: exe_nodefaultlib
	../../libtree exe

clean:
	rm -f *.so exe*
