CC		?= cc
CFLAGS		+= -g -Wall
CFLAGS		+= -fstack-protector

PREFIX		?= /usr/local
DESTDIR		?= $(PREFIX)

SRC		= attrdiff.c

BIN		?= ./attrdiff
BINUSR		?= root
BINGRP		?= wheel
BINMODE		?= 755

default: build

all: build install

build:
	$(CC) $(CFLAGS) $(SRC) -o $(BIN)

install: build
	install -o $(BINUSR) -g $(BINGRP) -m $(BINMODE) $(BIN) $(DESTDIR)/bin/attrdiff

uninstall:
	-rm $(DESTDIR)/bin/attrdiff

clean:
	-rm $(BIN)

.PHONY: uninstall clean

