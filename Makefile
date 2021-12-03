CFLAGS := -O2

all: libtree

libtree.o: libtree.c
	$(CC) $(CFLAGS) -c $?

libtree: libtree.o
	$(CC) $(CFLAGS) -o $@ $?

check: libtree
	for dir in $(wildcard tests/*); do \
		$(MAKE) -C $$dir check; \
	done

clean:
	rm -f *.o libtree
