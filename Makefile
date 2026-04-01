CC ?= cc
CFLAGS ?= -Wall -Wextra -pedantic

all: test test_fatal

test: test.c tripwire.c tripwire.h
	$(CC) $(CFLAGS) -o test test.c tripwire.c

test_fatal: test_fatal.c tripwire.c tripwire.h
	$(CC) $(CFLAGS) -o test_fatal test_fatal.c tripwire.c

run: test
	./test

run-all: test test_fatal
	./test
	./test_fatal

clean:
	rm -f test test_fatal

.PHONY: all run run-all clean
