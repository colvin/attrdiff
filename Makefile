CC	= cc
CFLAGS	+= -g -Wall

SRC	= attrdiff.c

all:
	$(CC) $(CFLAGS) $(SRC) -o attrdiff

clean:
	-rm attrdiff

.PHONY: clean

