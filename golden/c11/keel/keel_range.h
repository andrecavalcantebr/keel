/* keel/keel_range.h — o `.h` do módulo keel.range.
   `range` NÃO é `byref`: os verbos recebem cópia (linguagem §4.11). Declara os
   quatro que fazem dele contável e percorrível: first, limit, length, get. */
#ifndef KEEL_RANGE_H
#define KEEL_RANGE_H
#include "keel/prelude.h"
typedef struct keel_range { size_t first; size_t limit; } keel_range;
static inline keel_range keel_range_of(size_t a, size_t b) { return (keel_range){ a, b < a ? a : b }; }
static inline size_t keel_range_first (keel_range r) { return r.first; }
static inline size_t keel_range_limit (keel_range r) { return r.limit; }
static inline size_t keel_range_length(keel_range r) { return r.limit - r.first; }
static inline size_t keel_range_get   (keel_range r, size_t i) { return r.first + i; }
#endif
