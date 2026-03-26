# Roadmap

## Current state

The core concept is strong: a small, understandable, drop-in memory debugging
wrapper. The README communicates the value clearly, and the scope is
disciplined. Include a header, compile one `.c` file, get leak / overflow /
double-free checks.

The next jump is not adding more features. It is making the existing behavior
more correct and more defensible.

## Known issues

### `free(NULL)` crashes instead of being a no-op

Standard C defines `free(NULL)` as a no-op. `tripwire_free()` looks up the
pointer, fails to find it, and exits with `free(): NO BLOCK:`. This is
incompatible with normal C expectations.

### `realloc(NULL, size)` is not handled like standard C

Standard `realloc(NULL, size)` behaves like `malloc(size)`.
`tripwire_realloc()` requires the pointer to already exist in the tracker list
and aborts. Same issue with `realloc(p, 0)` which should behave like `free(p)`.

### Tracker metadata leaks by design

`tripwire_new()` allocates tracker nodes, and `tripwire_malloc()` /
`tripwire_realloc()` duplicate the file name. `tripwire_free()` only frees the
wrapped allocation, not the tracking node or copied filename. The debugger
itself leaks memory over time.

### `realloc()` leaves stale bookkeeping behind

`tripwire_realloc()` marks the old block as freed, creates a new tracker node,
and moves forward. This preserves history but grows internal metadata forever.
Needs an explicit design choice: preserve history for diagnostics, or maintain a
clean active-allocation table.

### No overflow checks on allocation arithmetic

`tripwire_malloc()` does `size + 2 * sizeof tw_sentinel`, and
`tripwire_calloc()` does `nmemb * size`, with no overflow guard.

### Not thread-safe

The global doubly linked list (`tw_head`, `tw_tail`) has no locking.
Multithreaded programs can corrupt the tracker or produce false reports.

### Test harness is POSIX-specific

`test.c` uses `unistd.h`, `fork()`, and `waitpid()`. The library aspires to be
portable but the tests are not.

### `memset` size mismatch may produce false positives

`tripwire_memset()` warns whenever the size differs from the full allocation,
even though partial `memset()` is often valid. Should be framed as a
"suspicious usage" warning if kept.

## Phase 1: Standard C compatibility

- `free(NULL)` becomes a no-op
- `realloc(NULL, size)` becomes `malloc(size)`
- `realloc(p, 0)` gets a defined documented policy
- Document behavior for untracked pointers

This moves the tool from prototype to usable drop-in debug helper.

## Phase 2: Tracker lifecycle cleanup

Decide on tracking memory strategy:

- Active-only registry, or history/quarantine mode

Then implement:

- Cleanup of tracker nodes and stored filename copies
- Optional final teardown function (`tripwire_cleanup()`)
- Clearer policy for freed vs active vs historical records

## Phase 3: Harden arithmetic and edge cases

- Overflow checks in `malloc` and `calloc` size arithmetic
- Tests for zero-size allocations
- Tests for `NULL` behavior
- Tests for repeated realloc chains

## Phase 4: Project credibility

- Separate demo tests from correctness tests
- Add portable test runner (no fork dependency)
- Add a "Limitations" section in the README
- Add examples showing where tripwire helps and where it does not

## Phase 5: Positioning and optional features

- Tighten README positioning: "lightweight debug allocator for small C
  projects, single-threaded, not a replacement for ASan/Valgrind"
- Optional thread-safety via mutex (compile-time flag)
- Optional quarantine mode for freed blocks (use-after-free detection)
