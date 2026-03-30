/* SPDX-License-Identifier: MIT */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char tw_sentinel[] = {'\001', '\002', '\003', '\004'};

typedef struct tripwire tripwire;
struct tripwire
{
    char *alloc;
    char *public;

    size_t allocated;

    const char *file;
    int line;

    int freed; // or reallocated

    tripwire *next;
    tripwire *prev;
};

void tripwire_free(void *, const char *, int);

static tripwire *tw_head = NULL;
static tripwire *tw_tail = NULL;

static void fatal1(const char *s, const char *file, int line)
{
    fprintf(stderr, "File: %s, Line: %d, \n", file, line);
    perror(s);
    exit(1);
}

static void fatal2(const char *s, const char *file, int line)
{
    fprintf(stderr, "%s: %s: %d\n", s, file, line);
    exit(1);
}

static void *tripwire_new(const char *file, int line)
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

void *tripwire_malloc(size_t size, const char *file, int line)
{
    tripwire *tw = tripwire_new(file, line);
    tw->alloc = malloc(size + 2 * sizeof tw_sentinel);
    if (tw->alloc == NULL) fatal1("malloc", file, line);

    tw->file = strdup(file);
    tw->line = line;

    tw->public = tw->alloc + sizeof tw_sentinel;
    tw->allocated = size;

    memcpy(tw->alloc, tw_sentinel, sizeof tw_sentinel);
    memcpy(tw->public + tw->allocated, tw_sentinel, sizeof tw_sentinel);

    return tw->public;
}

void *tripwire_realloc(void *p, size_t size, const char *file, int line)
{
    if (p == NULL)
	return tripwire_malloc(size, file, line);

    if (size == 0)
    {
	tripwire_free(p, file, line);
	return NULL;
    }

    tripwire *t = tripwire_find(p);
    if (t == NULL)
	fatal2("realloc(): NO BLOCK:", file, line);

    if (memcmp(t->alloc, tw_sentinel, sizeof tw_sentinel) != 0)
	fprintf(stderr, "SENTINEL 0 cracked in %s at %d\n",
		file, line);
    if (memcmp(t->public + t->allocated, tw_sentinel, sizeof tw_sentinel) != 0)
	fprintf(stderr, "SENTINEL 1 cracked in %s at %d\n",
		file, line);

    if (t->freed == 1) fatal2("Realloc of freed mem", file, line);
    t->freed = 1;

    tripwire *tw = tripwire_new(file, line);
    tw->alloc = realloc(t->alloc, size + 2 * sizeof tw_sentinel);
    if (tw->alloc == NULL) fatal1("malloc", file, line);

    tw->public = tw->alloc + sizeof tw_sentinel;
    tw->allocated = size;

    tw->file = strdup(file);
    tw->line = line;

    memcpy(tw->alloc, tw_sentinel, sizeof tw_sentinel);
    memcpy(tw->public + tw->allocated, tw_sentinel, sizeof tw_sentinel);

    return tw->public;
}

void tripwire_free(void *p, const char *file, int line)
{
    if (p == NULL) return;

    tripwire *t = tripwire_find(p);
    if (t == NULL)
	fatal2("free(): NO BLOCK:", file, line);

    if (t->freed == 1) fatal2("free(): DOUBLE FREE:", file, line);

    if (memcmp(t->alloc, tw_sentinel, sizeof tw_sentinel) != 0)
	fprintf(stderr, "SENTINEL 0 cracked in %s at %d\n",
		file, line);
    if (memcmp(t->public + t->allocated, tw_sentinel, sizeof tw_sentinel) != 0)
	fprintf(stderr, "SENTINEL 1 cracked in %s at %d\n",
		file, line);

    t->freed = 1;
    free(t->alloc);
    t->alloc = NULL;
}

void *tripwire_memset(void *p, int c, size_t l, const char *file, int line)
{
    tripwire *t = tripwire_find(p);
    if (t == NULL)
    {
	fprintf(stderr, "BLOCK NOT FOUND: memset(%p, %d, %zu) in %s at %d\n",
		p, c, l, file, line);
	return memset(p, c, l);
    }

    if (memcmp(t->alloc, tw_sentinel, sizeof tw_sentinel) != 0)
	fprintf(stderr, "SENTINEL 0 cracked in %s at %d\n",
		file, line);
    if (memcmp(t->public + t->allocated, tw_sentinel, sizeof tw_sentinel) != 0)
	fprintf(stderr, "SENTINEL 1 cracked in %s at %d\n",
		file, line);
    if (l != t->allocated)
	fprintf(stderr,
		"MEMSET mismatch: expected %zu got %zu in %s at %d\n",
		t->allocated, l, file, line);
    return memset(p, c, l);
}

void *tripwire_calloc(size_t nmemb, size_t size, const char *file, int line)
{
    void *p = tripwire_malloc(nmemb * size, file, line);
    memset(p, 0, nmemb * size);
    return p;
}

char *tripwire_strdup(const char *s, const char *file, int line)
{
    size_t len = strlen(s) + 1;
    char *p = tripwire_malloc(len, file, line);
    memcpy(p, s, len);
    return p;
}

void *tripwire_memcpy(void *dest, const void *src, size_t l, const char *file, int line)
{
    tripwire *t = tripwire_find(dest);
    if (t == NULL)
    {
	fprintf(stderr, "BLOCK NOT FOUND: memcpy(%p, %p, %zu) in %s at %d\n",
		dest, src, l, file, line);
	return memcpy(dest, src, l);
    }

    if (memcmp(t->alloc, tw_sentinel, sizeof tw_sentinel) != 0)
	fprintf(stderr, "SENTINEL 0 cracked in %s at %d\n",
		file, line);
    if (memcmp(t->public + t->allocated, tw_sentinel, sizeof tw_sentinel) != 0)
	fprintf(stderr, "SENTINEL 1 cracked in %s at %d\n",
		file, line);
    if (l > t->allocated)
	fprintf(stderr,
		"MEMCPY overflow: allocated %zu, copying %zu in %s at %d\n",
		t->allocated, l, file, line);
    return memcpy(dest, src, l);
}

void *tripwire_memmove(void *dest, const void *src, size_t l, const char *file, int line)
{
    tripwire *t = tripwire_find(dest);
    if (t == NULL)
    {
	fprintf(stderr, "BLOCK NOT FOUND: memmove(%p, %p, %zu) in %s at %d\n",
		dest, src, l, file, line);
	return memmove(dest, src, l);
    }

    if (memcmp(t->alloc, tw_sentinel, sizeof tw_sentinel) != 0)
	fprintf(stderr, "SENTINEL 0 cracked in %s at %d\n",
		file, line);
    if (memcmp(t->public + t->allocated, tw_sentinel, sizeof tw_sentinel) != 0)
	fprintf(stderr, "SENTINEL 1 cracked in %s at %d\n",
		file, line);
    if (l > t->allocated)
	fprintf(stderr,
		"MEMMOVE overflow: allocated %zu, moving %zu in %s at %d\n",
		t->allocated, l, file, line);
    return memmove(dest, src, l);
}

void tripwire_report(void)
{
    tripwire *t;
    int leaks = 0;

    for (t = tw_head; t != NULL; t = t->next)
    {
	if (t->freed) continue;

	if (memcmp(t->alloc, tw_sentinel, sizeof tw_sentinel) != 0)
	    fprintf(stderr, "SENTINEL 0 cracked: %s:%d (%zu bytes)\n",
		    t->file, t->line, t->allocated);
	if (memcmp(t->public + t->allocated, tw_sentinel, sizeof tw_sentinel) != 0)
	    fprintf(stderr, "SENTINEL 1 cracked: %s:%d (%zu bytes)\n",
		    t->file, t->line, t->allocated);

	fprintf(stderr, "LEAK: %zu bytes at %p allocated in %s:%d\n",
		t->allocated, (void *)t->public, t->file, t->line);
	leaks++;
    }

    if (leaks)
	fprintf(stderr, "tripwire: %d leak(s) detected\n", leaks);
}

void tripwire_cleanup(void)
{
    tripwire *t = tw_head;
    while (t != NULL)
    {
	tripwire *next = t->next;
	if (!t->freed)
	    free(t->alloc);
	free((void *)t->file);
	free(t);
	t = next;
    }
    tw_head = NULL;
    tw_tail = NULL;
}
