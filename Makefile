CC ?= cc
CFLAGS ?= -Wall -Wextra -pedantic

test: test.c tripwire.c tripwire.h
	$(CC) $(CFLAGS) -o test test.c tripwire.c

run: test
	./test

clean:
	rm -f test

.PHONY: run clean
