-include Make.user

CFLAGS ?= -O2
LIBTREE_CFLAGS = -std=c99 -Wall -Wextra -Wshadow -pedantic
LIBTREE_DEFINES = -D_FILE_OFFSET_BITS=64

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
SHAREDIR ?= $(PREFIX)/share

.PHONY: all check install clean

all: libtree

.c.o:
	$(CC) $(CFLAGS) $(LIBTREE_CFLAGS) $(LIBTREE_DEFINES) -c $<

libtree-objs = libtree.o
libtree: $(libtree-objs)
	$(CC) $(LDFLAGS) -o $@ $(libtree-objs)

install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	cp -p libtree $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(SHAREDIR)/man/man1
	cp -p doc/libtree.1 $(DESTDIR)$(SHAREDIR)/man/man1

check:: libtree

clean::
	rm -f *.o libtree

clean check::
	find tests -mindepth 1 -maxdepth 1 -type d | while read -r dir; do \
		$(MAKE) -C "$$dir" $@ || exit 1 ;\
	done
