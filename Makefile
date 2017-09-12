CC		= cc
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
	@echo "====> $@"
	$(CC) $(CFLAGS) $(SRC) -o $(BIN)

install: build
	@echo "====> $@"
	install -o $(BINUSR) -g $(BINGRP) -m $(BINMODE) $(BIN) $(DESTDIR)/bin/attrdiff

uninstall:
	@echo "====> $@"
	-rm $(DESTDIR)/bin/attrdiff

clean:
	@echo "====> $@"
	-rm $(BIN)

.PHONY: uninstall clean

