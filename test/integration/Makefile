all: src/main

CC := gcc

src/main: src/main.o src/a/libliba.so
	$(CC) '-Wl,--disable-new-dtags,-rpath,$$ORIGIN/a,-rpath-link,src/a,-rpath-link,src/a/b,-rpath-link,src/a/b/c' $< -o $@ -Lsrc/a -lliba

src/a/libliba.so: src/a/liba.o src/a/b/liblibb.so
	$(CC) '-Wl,--disable-new-dtags,-rpath,$$ORIGIN/b' -shared $< -o $@ -Lsrc/a/b -llibb

src/a/b/liblibb.so: src/a/b/libb.o src/a/b/c/liblibc.so
	$(CC) '-Wl,--enable-new-dtags,-rpath,$$ORIGIN/c' -shared $< -o $@ -Lsrc/a/b/c -llibc

src/a/b/c/liblibc.so: src/a/b/c/libc.o src/a/liblibd.so
	$(CC)  -shared $< -o $@ -Lsrc/a -llibd

src/a/liblibd.so: src/a/libd.o
	$(CC) -shared $< -o $@

%.o: %.c
	$(CC) -fPIC -c $< -o $@

clean:
	rm src/*.o src/a/*.so src/a/*.o src/a/b/*.so src/a/b/*.o src/a/b/c/*.so src/a/b/c/*.o
