/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef TRIPWIRE_H
#define TRIPWIRE_H

#include <stddef.h>

#define malloc(size) tripwire_malloc((size), __FILE__, __LINE__)
#define realloc(p, size) tripwire_realloc(p, (size), __FILE__, __LINE__)
#define calloc(n, size) tripwire_calloc((n), (size), __FILE__, __LINE__)
#define strdup(s) tripwire_strdup((s), __FILE__, __LINE__)
#undef memset
#undef memcpy
#undef memmove
#define memset(p, c, l) tripwire_memset(p, c, l, __FILE__, __LINE__)
#define memcpy(d, s, l) tripwire_memcpy(d, s, l, __FILE__, __LINE__)
#define memmove(d, s, l) tripwire_memmove(d, s, l, __FILE__, __LINE__)
#define free(p) tripwire_free(p, __FILE__, __LINE__)

void *tripwire_malloc(size_t, const char *, int);
void *tripwire_realloc(void *, size_t, const char *, int);
void *tripwire_calloc(size_t, size_t, const char *, int);
char *tripwire_strdup(const char *, const char *, int);
void *tripwire_memset(void *, int, size_t, const char *, int);
void *tripwire_memcpy(void *, const void *, size_t, const char *, int);
void *tripwire_memmove(void *, const void *, size_t, const char *, int);
void tripwire_free(void *, const char *, int);
void tripwire_report(void);

#endif
