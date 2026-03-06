/* Test program demonstrating all tripwire features. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "tripwire.h"

static void banner(const char *msg)
{
    fprintf(stderr, "\n=== %s ===\n", msg);
}

/* Run fn in a child process so fatal exits don't kill the test harness. */
static void run_fatal(const char *name, void (*fn)(void))
{
    banner(name);
    fflush(stderr);
    pid_t pid = fork();
    if (pid == 0) {
	fn();
	_exit(0);
    }
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
	fprintf(stderr, "  (caught -- child exited %d)\n",
		WEXITSTATUS(status));
}

/* --- Fatal test functions --- */

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

/* --- Main --- */

int main(void)
{
    /* Clean operations -- no warnings expected */

    banner("malloc + free");
    char *a = malloc(32);
    free(a);
    fprintf(stderr, "  (ok)\n");

    banner("calloc + free");
    int *b = calloc(10, sizeof(int));
    free(b);
    fprintf(stderr, "  (ok)\n");

    banner("strdup + free");
    char *c = strdup("hello tripwire");
    free(c);
    fprintf(stderr, "  (ok)\n");

    banner("realloc");
    char *d = malloc(16);
    d = realloc(d, 64);
    free(d);
    fprintf(stderr, "  (ok)\n");

    banner("memcpy (within bounds)");
    char *e = malloc(16);
    memcpy(e, "hello world", 11);
    free(e);
    fprintf(stderr, "  (ok)\n");

    banner("memmove (within bounds)");
    char *f = malloc(16);
    memcpy(f, "hello world!!!!!", 16);
    memmove(f, f + 1, 15);
    free(f);
    fprintf(stderr, "  (ok)\n");

    /* Warning tests -- tripwire detects and warns */

    banner("buffer overflow (trailing sentinel)");
    char *g = malloc(16);
    g[16] = 'X';		/* corrupt sentinel 1 */
    memset(g, 0, 16);		/* triggers sentinel check */
    free(g);

    banner("buffer underflow (leading sentinel)");
    char *h = malloc(16);
    h[-1] = 'X';		/* corrupt sentinel 0 */
    memset(h, 0, 16);		/* triggers sentinel check */
    free(h);

    banner("memset size mismatch");
    char *i = malloc(32);
    memset(i, 0, 16);		/* allocated 32, memset 16 */
    free(i);

    banner("memcpy overflow");
    char *j = malloc(8);
    memcpy(j, "too long!!!!",  12);	/* allocated 8, copying 12 */
    free(j);

    banner("memmove overflow");
    char *k = malloc(8);
    memmove(k, "too long!!!!", 12);	/* allocated 8, moving 12 */
    free(k);

    /* Fatal tests -- isolated in child processes */

    run_fatal("double free", do_double_free);
    run_fatal("free untracked pointer", do_free_untracked);

    /* Leak detection */

    banner("leak detection");
    char *leaked = malloc(128);
    (void)leaked;
    tripwire_report();

    return 0;
}
