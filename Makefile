#
#  Makefile for jhd - Japanese Hexdecimal Dump
#
#  jhd is a POSIX program: it uses isatty(), fileno() and fseeko().
#  C99 is enough (long long, strtoll()); the noreturn marker falls back to
#  __attribute__ when the compiler is not C11 yet.
#

CC	?= cc
RM	?= rm -f
CFLAGS	?= -O2
CPPFLAGS	+= -D_POSIX_C_SOURCE=200809L
WARNFLAGS	= -Wall -Wextra -Wstrict-prototypes -Wold-style-definition \
		  -Wmissing-prototypes -Wmissing-declarations
PROG	= jhd
OBJS	= jhd.o

PREFIX	= /usr/local
MANDIR	= $(PREFIX)/man

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(PROG) $(OBJS)

jhd.o: jhd.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNFLAGS) -c jhd.c

#  syntax check both Kanji code branches, so that the unused one does not rot
check:
	$(CC) $(CPPFLAGS) $(WARNFLAGS) -fsyntax-only jhd.c
	$(CC) $(CPPFLAGS) $(WARNFLAGS) -DEUC=0 -fsyntax-only jhd.c

install: $(PROG)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(PROG) $(DESTDIR)$(PREFIX)/bin/$(PROG)
	install -d $(DESTDIR)$(MANDIR)/man1
	install -m 644 jhd.1 $(DESTDIR)$(MANDIR)/man1/jhd.1

clean:
	$(RM) $(PROG) $(OBJS)

.PHONY: all check install clean
