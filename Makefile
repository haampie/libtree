CFLAGS := -O2

libtree.o: libtree.c
	$(CC) $(CFLAGS) -c $?

libtree: libtree.o
	$(CC) $(CFLAGS) -o $@ $?

clean:
	rm -f *.o libtree
