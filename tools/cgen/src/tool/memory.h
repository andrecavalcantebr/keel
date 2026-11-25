#ifndef CGEN_MEMORY_H
#define CGEN_MEMORY_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "keel/keel_arena.h"
#include "keel/keel_buffer_char.type.h"

/* Override at build time; one stable allocation for the whole invocation. */
#ifndef CGEN_ARENA_CAPACITY
#define CGEN_ARENA_CAPACITY (256u * 1024u * 1024u)
#endif

typedef keel_buffer_char string;
/* Borrowed mutable C string. cap includes the terminating zero. */
static inline string str_str(char *s) {
    size_t n = strlen(s);
    return (string){.len=n, .cap=n+1, .ptr=s};
}
static inline char *str_cstr(string s) { return s.ptr; }
/* Every allocation of the arena goes through here. The arena is one block, so the ASan sees no
 * redzone between two of its tables: an overflow from one into the next is invisible. A build with
 * -DCGEN_ASAN_REDZONE (make -C tools/cgen asan) puts a poisoned zone before and after each
 * allocation, so the overrun is reported where it happens (achados 2026-11-25, S1). */
#ifdef CGEN_ASAN_REDZONE
#include <sanitizer/asan_interface.h>
static inline void *cgen_arena_alloc(keel_arena *a, size_t size, size_t alignment, size_t n) {
    if (size == 0 || alignment == 0 || (alignment & (alignment - 1)) || n > (SIZE_MAX - 256) / size) return NULL;
    size_t bytes = n * size;
    void *raw = keel_arena_alloc_(a, 1, 16, 128 + 8 * alignment + bytes);
    if (!raw) return NULL;
    __asan_unpoison_memory_region(raw, 128 + 8 * alignment + bytes);   /* the space may have been poisoned by an earlier use */
    uintptr_t base = ((uintptr_t)raw + 64 + alignment - 1) & ~(uintptr_t)(alignment - 1);
    while (((base + bytes) & 7) != 0) base += alignment;   /* the end sits on an 8-byte granule */
    char *p = (char *)base;
    __asan_poison_memory_region(raw, (size_t)(p - (char *)raw));
    __asan_poison_memory_region(p + bytes, 64);
    return p;
}
#else
static inline void *cgen_arena_alloc(keel_arena *a, size_t size, size_t alignment, size_t n) {
    return keel_arena_alloc_(a, size, alignment, n);
}
#endif
/* Accepts a length-delimited source; returns an owned, terminated copy.
   A null pointer denotes failure, including for an empty input. */
static inline string str_dup(keel_arena *a, string orig) {
    if (orig.len == SIZE_MAX) return (string){0};
    char *p = (char *)cgen_arena_alloc(a, 1, 1, orig.len+1);
    if (!p) return (string){0};
    if (orig.len) memcpy(p, orig.ptr, orig.len);
    p[orig.len] = '\0';
    return (string){.len=orig.len, .cap=orig.len+1, .ptr=p};
}
/* Dev-only accounting, not part of the tool's interface: every allocation
   goes through cgen_alloc, so this tags each by its "what" and totals the
   bytes. Not a diagnostic, not documented anywhere a user reads; it exists so
   that measuring where CGEN_ARENA_CAPACITY actually goes doesn't need a
   profiler. CGEN_STAT_CAP distinct labels fit; more collapse into "other". */
#define CGEN_STAT_CAP 64
typedef struct { const char *what; size_t bytes; size_t calls; } CgenStat;
/* Defined once, in memory.c: a `static` here would give each including .c
   its own copy, and this must be the one table every TU adds to. */
extern CgenStat cgen_stats[CGEN_STAT_CAP];
extern size_t cgen_stats_n;

static inline void cgen_stat_add(const char *what, size_t bytes) {
    for (size_t i = 0; i < cgen_stats_n; ++i)
        if (strcmp(cgen_stats[i].what, what) == 0) { cgen_stats[i].bytes += bytes; cgen_stats[i].calls++; return; }
    if (cgen_stats_n == CGEN_STAT_CAP) what = "other (CGEN_STAT_CAP reached)";
    for (size_t i = 0; i < cgen_stats_n; ++i)
        if (strcmp(cgen_stats[i].what, what) == 0) { cgen_stats[i].bytes += bytes; cgen_stats[i].calls++; return; }
    cgen_stats[cgen_stats_n++] = (CgenStat){ what, bytes, 1 };
}
/* Sorted by bytes, descending; O(n^2) over at most CGEN_STAT_CAP entries. */
static inline void cgen_memory_report(FILE *f) {
    CgenStat sorted[CGEN_STAT_CAP];
    memcpy(sorted, cgen_stats, cgen_stats_n * sizeof(CgenStat));
    for (size_t i = 0; i < cgen_stats_n; ++i)
        for (size_t j = i + 1; j < cgen_stats_n; ++j)
            if (sorted[j].bytes > sorted[i].bytes) { CgenStat t = sorted[i]; sorted[i] = sorted[j]; sorted[j] = t; }
    fprintf(f, "cgen: arena usage by label (bytes, calls):\n");
    for (size_t i = 0; i < cgen_stats_n; ++i)
        fprintf(f, "  %10zu  %8zu  %s\n", sorted[i].bytes, sorted[i].calls, sorted[i].what);
}
static inline void cgen_memory_error(const keel_arena *a, const char *what) {
    fprintf(stderr, "cgen: error: arena capacity exhausted allocating %s; used %zu, configured limit %zu bytes; adjust CGEN_ARENA_CAPACITY [implementation-limit]\n",
            what, a->top, a->cap);
    cgen_memory_report(stderr);
}
static inline void *cgen_alloc(keel_arena *a, size_t n, size_t size,
                               size_t alignment, bool zero, const char *what) {
    size_t before = a->top;
    void *p = cgen_arena_alloc(a, size, alignment, n);
    if (!p) { cgen_memory_error(a, what); return NULL; }
    cgen_stat_add(what, a->top - before);
    if (zero) memset(p, 0, n*size);
    return p;
}
/* A table that one pass fills and nothing else allocates meanwhile, whose final size only that pass
 * knows: it is handed what the arena has left (at most `want` entries) and, once filled, the arena
 * keeps just the entries used. The table's worst case need not fit the arena, only what is written
 * (achados 2026-11-25, A5). Use it for the last allocation of the moment, never for a table that
 * later passes grow. */
static inline void *cgen_tail_reserve(keel_arena *a, size_t size, size_t alignment, size_t want, size_t *cap) {
    *cap = 0;
    if (a->top > a->cap || !a->ptr) return NULL;
    uintptr_t address = (uintptr_t)(a->ptr + a->top);
    size_t padding = (size_t)(-address & ((uintptr_t)alignment - 1));
    if (padding > a->cap - a->top) return NULL;
    size_t room = (a->cap - a->top - padding) / size;
    *cap = want < room ? want : room;
    return a->ptr + a->top + padding;
}
static inline void cgen_tail_commit(keel_arena *a, const void *table, size_t size, size_t used, const char *what) {
    size_t before = a->top;
    a->top = (size_t)((const unsigned char *)table - a->ptr) + used * size;
    cgen_stat_add(what, a->top - before);
#ifdef CGEN_ASAN_REDZONE
    __asan_poison_memory_region((const unsigned char *)table + used * size, 64);
#endif
}
#define CGEN_NEW(a,T,n) ((T *)cgen_alloc((a),(n),sizeof(T),_Alignof(T),true,#T))
static inline string cgen_string_dup(keel_arena *a, const char *p, size_t n) {
    string s = str_dup(a, (string){.ptr=(char *)p,.len=n,.cap=n});
    if (!s.ptr) cgen_memory_error(a, "string");
    return s;
}
static inline bool cgen_memory_init(keel_arena *a) {
    void *p = malloc(CGEN_ARENA_CAPACITY);
    if (!p) {
        fprintf(stderr,"cgen: error: cannot allocate invocation arena [out-of-memory]\n");
        *a=(keel_arena){0}; return false;
    }
    return keel_arena_from_memory(a,p,CGEN_ARENA_CAPACITY);
}
static inline void cgen_memory_destroy(keel_arena *a) {
    if (getenv("CGEN_ARENA_STATS")) cgen_memory_report(stderr);
    free(a->ptr); *a=(keel_arena){0};
}
#endif
