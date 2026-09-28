#ifndef CGEN_MEMORY_H
#define CGEN_MEMORY_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "keel/keel_arena.h"
#include "keel/keel_buffer_char.type.h"

/* Override at build time; one stable allocation for the whole invocation. */
#ifndef CGEN_ARENA_CAPACITY
#define CGEN_ARENA_CAPACITY (64u * 1024u * 1024u)
#endif

typedef keel_buffer_char string;
/* Borrowed mutable C string. cap includes the terminating zero. */
static inline string str_str(char *s) {
    size_t n = strlen(s);
    return (string){.len=n, .cap=n+1, .ptr=s};
}
static inline char *str_cstr(string s) { return s.ptr; }
/* Accepts a length-delimited source; returns an owned, terminated copy.
   A null pointer denotes failure, including for an empty input. */
static inline string str_dup(keel_arena *a, string orig) {
    if (orig.len == SIZE_MAX) return (string){0};
    char *p = keel_arena_alloc(a, char, orig.len+1);
    if (!p) return (string){0};
    if (orig.len) memcpy(p, orig.ptr, orig.len);
    p[orig.len] = '\0';
    return (string){.len=orig.len, .cap=orig.len+1, .ptr=p};
}
static inline void cgen_memory_error(const keel_arena *a, const char *what) {
    fprintf(stderr, "cgen: error: arena capacity exhausted allocating %s; used %zu, configured limit %zu bytes; adjust CGEN_ARENA_CAPACITY [implementation-limit]\n",
            what, a->top, a->cap);
}
static inline void *cgen_alloc(keel_arena *a, size_t n, size_t size,
                               size_t alignment, bool zero, const char *what) {
    void *p = keel_arena_alloc_(a, size, alignment, n);
    if (!p) { cgen_memory_error(a, what); return NULL; }
    if (zero) memset(p, 0, n*size);
    return p;
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
    free(a->ptr); *a=(keel_arena){0};
}
#endif
