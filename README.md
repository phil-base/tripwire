# Tripwire

Lightweight C memory debugging wrapper. Include the header and it transparently
replaces standard allocation and memory functions via preprocessor macros.

No compiler flags, no linking extra libraries, no external tools.

## What It Detects

- **Buffer overflows/underflows** -- sentinel bytes before and after each
  allocation are checked on realloc, free, memset, memcpy, and memmove
- **Double frees** -- tracked via a flag on each allocation
- **Invalid pointer operations** -- realloc/free of untracked pointers
- **memset size mismatches** -- warns when memset size differs from allocation
- **memcpy/memmove overflows** -- warns when copy size exceeds allocation
- **Memory leaks** -- `tripwire_report()` walks the allocation list and reports
  unfreed blocks

## Usage

Include `tripwire.h` after the standard headers in your source files, and
compile with `tripwire.c`:

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tripwire.h"

int main(void)
{
    char *buf = malloc(10);
    // use buf...
    free(buf);

    tripwire_report(); // print any leaks
    return 0;
}
```

```sh
cc -o myprogram myprogram.c tripwire.c
```

## Example Output

Buffer overflow (sentinel corrupted):
```
SENTINEL 1 cracked in main.c at 23
```

memset size doesn't match allocation:
```
MEMSET mismatch: expected 32 got 64 in main.c at 25
```

Double free:
```
free(): DOUBLE FREE:: main.c: 30
```

Leak report from `tripwire_report()`:
```
LEAK: 128 bytes at 0x600000000010 allocated in parser.c:42
LEAK: 64 bytes at 0x600000000120 allocated in parser.c:58
tripwire: 2 leak(s) detected
```

"SENTINEL 0" means the bytes *before* the allocation were corrupted (underflow).
"SENTINEL 1" means the bytes *after* the allocation were corrupted (overflow).

## Wrapped Functions

| Standard call | Tripwire replacement |
|---|---|
| `malloc(size)` | Adds sentinels, tracks allocation |
| `realloc(p, size)` | Validates pointer, checks sentinels, re-wraps |
| `calloc(n, size)` | Tracked zero-initialized allocation |
| `strdup(s)` | Tracked string duplication |
| `free(p)` | Validates pointer, detects double-free |
| `memset(p, c, l)` | Checks sentinels and size match |
| `memcpy(d, s, l)` | Checks sentinels and overflow |
| `memmove(d, s, l)` | Checks sentinels and overflow |

## License

BSD 2-Clause. See [LICENSE](LICENSE).
