# This creates exe <- liba.so <- libb.so There are two libraries:
# {first,second}/libb.so and exe has an rpath to first, whereas liba.so has an
# rpath to second

LD_LIBRARY_PATH=

.PHONY: clean

all: check

first/libb.so: 
	@mkdir -p $(@D)
	echo 'int g(){return 1;}' | $(CC) -shared -Wl,-soname,$(@F) -o $@ -x c -

second/libb.so: 
	@mkdir -p $(@D)
	echo 'int g(){return 2;}' | $(CC) -shared -Wl,-soname,$(@F) -o $@ -x c -

liba.so: second/libb.so
	echo 'extern int g(); int f(){return g();}' | $(CC) -shared -Wl,--disable-new-dtags -Wl,-soname,$@ -o $@ '-Wl,-rpath,$$ORIGIN/second' -x c - -Lsecond -lb

exe: first/libb.so liba.so
	echo 'extern int f(); int main(){return f();}' | $(CC) -o $@ -Wl,--disable-new-dtags '-Wl,-rpath,$$ORIGIN,-rpath,$$ORIGIN/first' -x c - -L. -la

check: exe liba.so
	../../libtree -p liba.so  # should find second/libb.so, not first/libb.so
	../../libtree -p exe  # should find second/libb.so, not first/libb.so

clean:
	rm -rf -- *.so exe* first second
