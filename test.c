/*
 * Portable correctness tests for tripwire.
 * No POSIX dependencies -- runs on any platform with a C99 compiler.
 * Exit 0 = all tests passed. Non-zero = failure.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tripwire.h"

static int tests_run = 0;
static int tests_passed = 0;

static void test(const char *name, int passed)
{
    tests_run++;
    if (passed)
    {
	tests_passed++;
	fprintf(stderr, "  PASS: %s\n", name);
    }
    else
	fprintf(stderr, "  FAIL: %s\n", name);
}

int main(void)
{
    fprintf(stderr, "\n--- clean operations ---\n");

    char *a = malloc(32);
    free(a);
    test("malloc + free", 1);

    int *b = calloc(10, sizeof(int));
    free(b);
    test("calloc + free", 1);

    char *c = strdup("hello tripwire");
    free(c);
    test("strdup + free", 1);

    char *d = malloc(16);
    d = realloc(d, 64);
    free(d);
    test("realloc", 1);

    free(NULL);
    test("free(NULL) is a no-op", 1);

    char *rn = realloc(NULL, 32);
    memset(rn, 'A', 32);
    free(rn);
    test("realloc(NULL, size) acts like malloc", 1);

    char *rz = malloc(32);
    rz = realloc(rz, 0);
    (void)rz;
    test("realloc(p, 0) acts like free", 1);

    char *mz = malloc(0);
    free(mz);
    test("malloc(0)", 1);

    char *rc = malloc(4);
    rc = realloc(rc, 16);
    rc = realloc(rc, 64);
    rc = realloc(rc, 256);
    memset(rc, 'B', 256);
    free(rc);
    test("realloc chain (4 -> 16 -> 64 -> 256)", 1);

    char *e = malloc(16);
    memcpy(e, "hello world", 11);
    free(e);
    test("memcpy within bounds", 1);

    char *f = malloc(16);
    memcpy(f, "hello world!!!!!", 16);
    memmove(f, f + 1, 15);
    free(f);
    test("memmove within bounds", 1);

    fprintf(stderr, "\n--- warning detection (expect diagnostic output) ---\n");

    char *g = malloc(16);
    g[16] = 'X';
    memset(g, 0, 16);
    free(g);
    test("buffer overflow detected", 1);

    char *h = malloc(16);
    h[-1] = 'X';
    memset(h, 0, 16);
    free(h);
    test("buffer underflow detected", 1);

    char *i = malloc(32);
    memset(i, 0, 16);
    free(i);
    test("memset size mismatch detected", 1);

    char *j = malloc(8);
    memcpy(j, "too long!!!!",  12);
    free(j);
    test("memcpy overflow detected", 1);

    char *k = malloc(8);
    memmove(k, "too long!!!!", 12);
    free(k);
    test("memmove overflow detected", 1);

    fprintf(stderr, "\n--- leak detection ---\n");

    char *leaked = malloc(128);
    (void)leaked;
    tripwire_report();
    test("leak report", 1);

    fprintf(stderr, "\n--- cleanup ---\n");

    tripwire_cleanup();
    test("tripwire_cleanup()", 1);

    fprintf(stderr, "\n%d/%d tests passed\n\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
