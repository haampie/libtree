# When passing -z nodefaultlib, the runtime linker should not consider
# ld.so.conf nor standard paths (/usr/lib, /lib, ...). This is a little bit
# hard to test but in general it means that if you use the system compiler with
# this flag, libc itself would not be located.
#
# There's two tests here:
# 1. exe_a (itself nodefaultlib) depends on a lib with and a lib without -z
#    nodefaultlib
# 2. exe_b (itself nodefaultlib) depends on a lib without nodefaultlib.
#
# The question is whether or not libc can be located as a lib in the default
# paths. (The expected behavior is not well documented...)

.PHONY: clean check

LD_LIBRARY_PATH=

all: check

lib_nodefaultlib.so: 
	echo 'int f(){return 1;}' | $(CC) -z nodefaultlib -Wl,--no-as-needed -shared -Wl,-soname,$@ -o $@ -x c -

lib_defaultlib.so:
	echo 'int g(){return 1;}' | $(CC) -Wl,--no-as-needed -shared -Wl,-soname,$@ -o $@ -x c -

exe_a: lib_nodefaultlib.so lib_defaultlib.so
	echo 'extern int f(); extern int g(); int main(){return f() + g();}' | $(CC) -z nodefaultlib -o $@ -Wl,--enable-new-dtags '-Wl,-rpath,$$ORIGIN' -x c - -L. -l_nodefaultlib -l_defaultlib

exe_b: lib_defaultlib.so
	echo 'extern int g(); int main(){return g();}' | $(CC) -z nodefaultlib -o $@ -Wl,--disable-new-dtags '-Wl,-rpath,$$ORIGIN' -x c - -L. -l_defaultlib

check: exe_a exe_b
	! ../../libtree -vvv exe_a # should likely not find libc
	! ../../libtree -vvv exe_b # should likely not find libc

clean:
	rm -f *.so exe*
