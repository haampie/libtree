CFLAGS := -O2

all: libtree

libtree.o: libtree.c
	$(CC) $(CFLAGS) -c $?

libtree: libtree.o
	$(CC) $(CFLAGS) -o $@ $?

check_a: libtree
	./libtree ./example/executable_a

check_b: libtree
	./libtree ./example/executable_b

check: check_a check_b

clean:
	rm -f *.o libtree
