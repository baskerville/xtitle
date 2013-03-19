VERSION = 0.1

CC      = gcc
LIBS    = -lm -lxcb -lxcb-icccm -lxcb-ewmh
CFLAGS  = -std=c99 -pedantic -Wall -Wextra -I$(PREFIX)/include
CFLAGS  += -D_POSIX_C_SOURCE=200112L -DVERSION=\"$(VERSION)\"
LDFLAGS = -L$(PREFIX)/lib

PREFIX    ?= /usr/local
BINPREFIX = $(PREFIX)/bin

SRC = xtitle.c helpers.c
HDR = $(SRC:.c=.h)
OBJ = $(SRC:.c=.o)

all: CFLAGS += -Os
all: LDFLAGS += -s
all: xtitle

$(OBJ): $(SRC) $(HDR) Makefile

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

xtitle: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS) $(LIBS)

install:
	mkdir -p "$(DESTDIR)$(BINPREFIX)"
	cp xtitle "$(DESTDIR)$(BINPREFIX)"
	chmod 755 "$(DESTDIR)$(BINPREFIX)/xtitle"

uninstall:
	rm -f $(DESTDIR)$(BINPREFIX)/xtitle

clean:
	rm -f $(OBJ) xtitle

.PHONY: all clean install uninstall
