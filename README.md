# Tripwire

[![CI](https://github.com/phil-base/tripwire/actions/workflows/ci.yml/badge.svg)](https://github.com/phil-base/tripwire/actions/workflows/ci.yml)

Lightweight C memory debugging wrapper. Include the header and it transparently
replaces standard allocation and memory functions via preprocessor macros.

No compiler flags, no linking extra libraries, no external tools.

## Why Tripwire?

| | Tripwire | ASan | Valgrind | Electric Fence |
|---|---|---|---|---|
| Setup | Copy two files | Compiler flag (`-fsanitize=address`) | External binary | Link library |
| Compiler support | Any C89+ compiler | GCC/Clang only | N/A (runtime) | GCC/Clang |
| Platform | Anywhere `malloc` works | Linux, macOS, some BSDs | Linux (best), macOS (limited) | Linux |
| Overhead | Minimal (sentinel bytes) | ~2x slowdown, ~3x memory | ~10-20x slowdown | One page per allocation |
| Leak detection | Yes (`tripwire_report()`) | Yes | Yes | No |
| Buffer overflow | Yes (sentinel canaries) | Yes (shadow memory) | Yes | Yes (page guards) |
| Double free | Yes | Yes | Yes | No |
| Source location | File and line number | File, line, stack trace | Stack trace | Signal only |
| Integration | `#include "tripwire.h"` | Rebuild with flag | Run under `valgrind` | Link with `-lefence` |

**Use tripwire when** you want quick, portable memory checks with zero setup
cost -- especially in environments where ASan or Valgrind aren't available
(embedded targets, unfamiliar toolchains, quick debugging sessions).

**Use ASan/Valgrind when** you need thorough instrumentation, stack traces, or
are debugging production-grade software on a supported platform.

## Installation

Copy `tripwire.h` and `tripwire.c` into your project:

```sh
# As a file copy
cp tripwire.h tripwire.c /path/to/your/project/

# Or as a git submodule
git submodule add https://github.com/phil-base/tripwire.git vendor/tripwire
```

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

    tripwire_report();  // print any leaks
    tripwire_cleanup(); // free all internal tracking state
    return 0;
}
```

```sh
cc -o myprogram myprogram.c tripwire.c
```

Or use the included Makefile to build and run the test suite:

```sh
make run
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
| `free(p)` | Validates pointer, checks sentinels, detects double-free |
| `memset(p, c, l)` | Checks sentinels and size match |
| `memcpy(d, s, l)` | Checks sentinels and overflow |
| `memmove(d, s, l)` | Checks sentinels and overflow |

## Diagnostics

| Function | Purpose |
|---|---|
| `tripwire_report()` | Print all unfreed allocations with file and line |
| `tripwire_cleanup()` | Free all internal tracking state (call after report) |

## Limitations

- **Not thread-safe.** The internal allocation list has no locking. Use
  tripwire in single-threaded programs or single-threaded test harnesses.
- **Sentinel-based detection only.** Tripwire uses 4-byte canary values before
  and after each allocation. It catches overwrites that hit these bytes but
  cannot detect arbitrary out-of-bounds access the way ASan's shadow memory can.
- **No stack or global checking.** Only heap allocations (`malloc`, `calloc`,
  `realloc`, `strdup`) are tracked. Stack buffer overflows and global variable
  issues are invisible to tripwire.
- **`memset` size mismatch is a heuristic.** Tripwire warns when the `memset`
  size differs from the allocation size. Partial `memset` on a buffer is valid C
  but will trigger a warning.
- **Tracker metadata grows over the lifetime of the program.** Freed allocation
  records are kept for double-free detection. Call `tripwire_cleanup()` at
  program exit to release them.

## License

MIT. See [LICENSE](LICENSE).
