-include Make.user

CFLAGS ?= -O2
LIBTREE_CFLAGS = -std=c99 -Wall -Wextra -Wshadow -pedantic
LIBTREE_DEFINES = -D_FILE_OFFSET_BITS=64

# uppercase variables for backwards compatibility only:
PREFIX = /usr/local

prefix = $(PREFIX)
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin
datarootdir = $(prefix)/share
mandir = $(datarootdir)/man

.PHONY: all check install clean

all: libtree

.c.o:
	$(CC) $(CFLAGS) $(LIBTREE_CFLAGS) $(LIBTREE_DEFINES) -c $<

libtree-objs = libtree.o
libtree: $(libtree-objs)
	$(CC) $(LDFLAGS) -o $@ $(libtree-objs)

install: all
	mkdir -p $(DESTDIR)$(bindir)
	cp -p libtree $(DESTDIR)$(bindir)
	mkdir -p $(DESTDIR)$(mandir)/man1
	cp -p doc/libtree.1 $(DESTDIR)$(mandir)/man1

check:: libtree

clean::
	rm -f *.o libtree

clean check::
	find tests -mindepth 1 -maxdepth 1 -type d | while read -r dir; do \
		$(MAKE) -C "$$dir" $@ || exit 1 ;\
	done
