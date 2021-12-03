LD_LIBRARY_PATH:=

.PHONY: clean

all: check

some_dir/lib_f.so:
	mkdir some_dir
	echo 'int f(){return 42;}' | $(CC) -shared -Wl,-soname,$(notdir $@) -Wl,--no-as-needed -o $@ -x c -

lib_g.so: some_dir/lib_f.so
	echo 'extern int f(); int g(){return f();}' | $(CC) -shared -Wl,-soname,$@ -Wl,--no-as-needed "-Wl,-rpath,$(CURDIR)/some_dir" -L./some_dir -l_f -o $@ -x c -

lib_without_soname.so:
	echo 'int i(){return 1;}' | $(CC) -shared -Wl,--no-as-needed -o $@ -x c -

exe_a: some_dir/lib_f.so lib_g.so lib_without_soname.so
	echo 'extern int i(); extern int f(); extern int g(); int main(){return f() + g() + i();}' | $(CC) -Wl,--no-as-needed "-Wl,-rpath,$(CURDIR)/" -L. -L./some_dir -l_f -l_g $(CURDIR)/lib_without_soname.so -o $@ -x c -

exe_b: some_dir/lib_f.so lib_g.so lib_without_soname.so
	echo 'extern int i(); extern int f(); extern int g(); int main(){return f() + g() + i();}' | $(CC) -Wl,--no-as-needed "-Wl,-rpath,$(CURDIR)/" "-Wl,-rpath,$(CURDIR)/some_dir" -L. -L./some_dir -l_f -l_g $(CURDIR)/lib_without_soname.so -o $@ -x c -

check: exe_a exe_b
	../../libtree exe_a  # cannot find lib_f.so
	../../libtree exe_b  # should find lib_f.so

clean:
	rm -rf *.so some_dir exe*

