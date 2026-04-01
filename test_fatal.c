/*
 * Fatal-path tests for tripwire (POSIX only).
 * Uses fork() to isolate tests that call exit().
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "tripwire.h"

static int tests_run = 0;
static int tests_passed = 0;

/* Run fn in a child process; expect it to exit with non-zero status. */
static void expect_fatal(const char *name, void (*fn)(void))
{
    tests_run++;
    fflush(stdout);
    fflush(stderr);
    pid_t pid = fork();
    if (pid == 0) {
	fn();
	_exit(0);
    }
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
    {
	tests_passed++;
	fprintf(stderr, "  PASS: %s (child exited %d)\n",
		name, WEXITSTATUS(status));
    }
    else
	fprintf(stderr, "  FAIL: %s (expected fatal exit, got status %d)\n",
		name, status);
}

static void do_double_free(void)
{
    char *p = malloc(16);
    free(p);
    free(p);
}

static void do_free_untracked(void)
{
    int x;
    free(&x);
}

static void do_calloc_overflow(void)
{
    void *p = calloc((size_t)-1 / 2, 3);
    (void)p;
}

static void do_realloc_untracked(void)
{
    int x;
    void *p = realloc(&x, 32);
    (void)p;
}

int main(void)
{
    fprintf(stderr, "\n--- fatal path tests (POSIX) ---\n");

    expect_fatal("double free", do_double_free);
    expect_fatal("free untracked pointer", do_free_untracked);
    expect_fatal("calloc overflow", do_calloc_overflow);
    expect_fatal("realloc untracked pointer", do_realloc_untracked);

    fprintf(stderr, "\n%d/%d tests passed\n\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
