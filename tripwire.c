#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *tw_sentinel = "\001\002\003\004";

typedef struct tripwire tripwire;
struct tripwire
{
    void *alloc;
    void *public;

    size_t allocated;

    char *file;
    int line;

    int freed; // or reallocated

    tripwire *next;
    tripwire *prev;
};

static tripwire *tw_head = NULL;
static tripwire *tw_tail = NULL;

static void fatal1(char *s, char *file, int line)
{
    fprintf(stderr, "File: %s, Line: %d, \n", file, line);
    perror(s);
    exit(1);
}

static void fatal2(char *s, char *file, int line)
{
    fprintf(stderr, "%s: %s: %d\n", s, file, line);
    exit(1);
}

static void *tripwire_new(char *file, int line)
{
    tripwire *tw = malloc(sizeof(tripwire));
    if (tw == NULL) fatal1("malloc", file, line);
    memset(tw, 0, sizeof(tripwire));

    if (tw_head == NULL)
	tw_head = tw;
    if (tw_tail != NULL)
    {
	tw->prev = tw_tail;
	tw_tail->next = tw;
    }
    tw_tail = tw;

    return tw;
}

static void *tripwire_find(void *p)
{
    tripwire *t;

    // search last created first - important
    for (t = tw_tail; t != NULL; t = t->prev)
	if (p == t->public)
	    return t;
    return NULL;
}

void *tripwire_malloc(size_t size, char *file, int line)
{
    tripwire *tw = tripwire_new(file, line);
    tw->alloc = malloc(size + 2 * sizeof tw_sentinel);
    if (tw->alloc == NULL) fatal1("malloc", file, line);

    tw->file = strdup(file);
    tw->line = line;

    tw->public = tw->alloc + 4;
    tw->allocated = size;

    memcpy(tw->alloc, tw_sentinel, sizeof tw_sentinel);
    memcpy(tw->public + tw->allocated, tw_sentinel, sizeof tw_sentinel);

    return tw->public;
}

void *tripwire_realloc(void *p, size_t size, char *file, int line)
{
    tripwire *t = tripwire_find(p);
    if (t == NULL)
	fatal2("realloc()(): NO BLOCK:", file, line);

    if (memcmp(t->alloc, tw_sentinel, sizeof tw_sentinel) != 0)
	fprintf(stderr, "SENTINEL 0 cracked in %s at %dd\n",
		file, line);
    if (memcmp(t->public + t->allocated, tw_sentinel, sizeof tw_sentinel) != 0)
	fprintf(stderr, "SENTINEL 1 cracked in %s at %dd\n",
		file, line);

    if (t->freed == 1) fatal2("Realloc of freed mem", file, line);
    t->freed = 1;

    tripwire *tw = tripwire_new(file, line);
    tw->alloc = realloc(t->alloc, size + 2 * sizeof tw_sentinel);
    if (tw->alloc == NULL) fatal1("malloc", file, line);

    tw->public = tw->alloc + 4;
    tw->allocated = size;

    return tw->public;
}

void tripwire_free(void *p, char *file, int line)
{
    tripwire *t = tripwire_find(p);
    if (t == NULL)
	fatal2("free()(): NO BLOCK:", file, line);

    if (t->freed == 1) fatal2("free(): DOUBLE FREE:", file, line);
    t->freed = 1;
    free(t->alloc);
}

void *tripwire_memset(void *p, int c, size_t l, char *file, int line)
{
    tripwire *t = tripwire_find(p);
    if (t == NULL)
    {
	fprintf(stderr, "BLOCK NOT FOUND: memset(0x%lx, %d, %d) in %s at %d\n",
		p, c, l, file, line);
	return memset(p, c, l);
    }

    if (memcmp(t->alloc, tw_sentinel, sizeof tw_sentinel) != 0)
	fprintf(stderr, "SENTINEL 0 cracked in %s at %dd\n",
		file, line);
    if (memcmp(t->public + t->allocated, tw_sentinel, sizeof tw_sentinel) != 0)
	fprintf(stderr, "SENTINEL 1 cracked in %s at %dd\n",
		file, line);
    if (l != t->allocated)
	fprintf(stderr,
		"MEMSET mismatch: expected %d got %d in %s at %d\n",
		t->allocated, l, file, line);
    return memset(p, c, l);
}
