ifeq (exists, $(shell [ -e $(CURDIR)/Make.user ] && echo exists ))
include $(CURDIR)/Make.user
endif

# we're using fileno and getline gnu extensions
LIBTREE_CFLAGS := -O2 -std=gnu99 -D_FILE_OFFSET_BITS=64 $(CFLAGS)

all: libtree

%.o: %.c
	$(CC) $(LIBTREE_CFLAGS) -c $?

libtree: libtree.o
	$(CC) $(LIBTREE_CFLAGS) -o $@ $?

check: libtree
	for dir in $(wildcard tests/*); do \
		$(MAKE) -C $$dir check; \
	done

clean:
	rm -f *.o libtree

