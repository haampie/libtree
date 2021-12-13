ifeq (exists, $(shell [ -e $(CURDIR)/Make.user ] && echo exists ))
include $(CURDIR)/Make.user
endif

CFLAGS ?= -O2
LIBTREE_CFLAGS := -std=c99 -Wall -Wextra -Wshadow -pedantic
LIBTREE_DEFINES := -D_FILE_OFFSET_BITS=64

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

.PHONY: all check install clean

all: libtree

%.o: %.c
	$(CC) $(CFLAGS) $(LIBTREE_CFLAGS) $(LIBTREE_DEFINES) -c $?

libtree: libtree.o
	$(CC) $(LDFLAGS) $^ -o $@

check: libtree
	for dir in $(sort $(wildcard tests/*)); do \
		$(MAKE) -C $$dir check; \
	done

install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	cp libtree $(DESTDIR)$(BINDIR)

clean:
	rm -f *.o libtree
