ifeq (exists, $(shell [ -e $(CURDIR)/Make.user ] && echo exists ))
include $(CURDIR)/Make.user
endif

CFLAGS ?= -O2
LIBTREE_CFLAGS := $(CFLAGS) -Wall -std=c99 -D_FILE_OFFSET_BITS=64

all: libtree

%.o: %.c
	$(CC) $(LIBTREE_CFLAGS) -c $?

libtree: libtree.o
	$(CC) $(LDFLAGS) $^ -o $@

check: libtree
	for dir in $(sort $(wildcard tests/*)); do \
		$(MAKE) -C $$dir check; \
	done

clean:
	rm -f *.o libtree

