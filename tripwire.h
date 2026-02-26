#define malloc(size) tripwire_malloc((size), __FILE__, __LINE__)
#define realloc(p, size) tripwire_realloc(p, (size), __FILE__, __LINE__)
#define memset(p, c, l) tripwire_memset(p, c, l, __FILE__, __LINE__)
#define free(p) tripwire_free(p, __FILE__, __LINE__)

void *tripwire_malloc(size_t, char *, int);
void *tripwire_realloc(void *, size_t, char *, int);
void *tripwire_memset(void *, int, size_t, char *, int);
void tripwire_free(void *, char *, int);
