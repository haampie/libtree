all: executable_a executable_b

lib_f.so: some_dir
	echo 'int f(){return 42;}' | $(CC) -shared -Wl,-soname,$@ -Wl,--no-as-needed -o some_dir/$@ -x c -

lib_g.so: lib_f.so
	echo 'extern int f(); int g(){return f();}' | $(CC) -shared -Wl,-soname,$@ -Wl,--no-as-needed "-Wl,-rpath,$(CURDIR)/some_dir" -L./some_dir -l_f -o $@ -x c -

lib_without_soname.so:
	echo 'int i(){return 1;}' | $(CC) -shared -Wl,--no-as-needed -o $@ -x c -

# executable cannot locate libf directly
executable_a: lib_f.so lib_g.so lib_without_soname.so
	echo 'extern int i(); extern int f(); extern int g(); int main(){return f() + g() + i();}' | $(CC) -Wl,--no-as-needed "-Wl,-rpath,$(CURDIR)/" -L. -L./some_dir -l_f -l_g $(CURDIR)/lib_without_soname.so -o $@ -x c -

# executable can locate all
executable_b: lib_f.so lib_g.so lib_without_soname.so
	echo 'extern int i(); extern int f(); extern int g(); int main(){return f() + g() + i();}' | $(CC) -Wl,--no-as-needed "-Wl,-rpath,$(CURDIR)/" "-Wl,-rpath,$(CURDIR)/some_dir" -L. -L./some_dir -l_f -l_g $(CURDIR)/lib_without_soname.so -o $@ -x c -

some_dir: 
	mkdir $@

clean:
	rm -rf *.so some_dir/ executable_*

