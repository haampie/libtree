# Test libtree --sysroot <prefix>

.PHONY: clean check

LD_LIBRARY_PATH:=

all: check

# this one should be found in the default paths
usr/lib/libdefaultpath.so: 
	@mkdir -p $(dir $@)
	echo 'int a(){return 1;}' | $(CC) -nostdlib -shared "-Wl,-soname,$(notdir $@)" -o $@ -x c -

# this one should be found in the rpaths
usr/lib/local/rpath/librpath.so: 
	@mkdir -p $(dir $@)
	echo 'int b(){return 2;}' | $(CC) -nostdlib -shared "-Wl,-soname,$(notdir $@)" -o $@ -x c -

exe_rpath: usr/lib/libdefaultpath.so usr/lib/local/rpath/librpath.so
	echo 'extern int a(); extern int b(); int _start(){return a()+b();}' | $(CC) \
		-Wl,--disable-new-dtags \
		-Wl,-rpath,/$(dir $(word 2,$^)) \
		-nostdlib -o $@ -x c - \
		-L$(dir $(word 1,$^)) -ldefaultpath \
		-L$(dir $(word 2,$^)) -lrpath \

exe_runpath: usr/lib/libdefaultpath.so usr/lib/local/rpath/librpath.so
	echo 'extern int a(); extern int b(); int _start(){return a()+b();}' | $(CC) \
		-Wl,--enable-new-dtags \
		-Wl,-rpath,/$(dir $(word 2,$^)) \
		-nostdlib -o $@ -x c - \
		-L$(dir $(word 1,$^)) -ldefaultpath \
		-L$(dir $(word 2,$^)) -lrpath \

check: exe_rpath exe_runpath
	../../libtree -p --sysroot . $(word 1,$^)
	../../libtree -p --sysroot $(CURDIR) $(word 1,$^)
	../../libtree -p --sysroot . $(word 2,$^)
	../../libtree -p --sysroot $(CURDIR) $(word 2,$^)

clean:
	rm -rf -- usr/ exe*
