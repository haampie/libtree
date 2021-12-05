ifeq (exists, $(shell [ -e $(CURDIR)/Make.user ] && echo exists ))
include $(CURDIR)/Make.user
endif

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

