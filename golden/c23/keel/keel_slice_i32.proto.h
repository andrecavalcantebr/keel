/* keel/keel_slice_i32.proto.h — gerado de keel/slice.k pelo cgen, perfil C23. */
#ifndef KEEL_KEEL_SLICE_I32_PROTO_H
#define KEEL_KEEL_SLICE_I32_PROTO_H
#include "keel/keel_slice_i32.type.h"
#include "keel/keel_slice.type.h"

static inline keel_slice_i32 keel_slice_i32_from(i32 *p, size_t n);
static inline size_t keel_slice_i32_length(keel_slice_i32 s);
static inline i32    keel_slice_i32_get (keel_slice_i32 s, size_t i);
static inline i32   *keel_slice_i32_ptr (keel_slice_i32 s);
static inline i32   *keel_slice_i32_ptr1(keel_slice_i32 s, size_t i);
/* cursor e partição recebem a cópia, como todo verbo de `slice` */
static inline keel_slice_cursor keel_slice_i32_begin(keel_slice_i32 s);
static inline bool keel_slice_i32_has_next(keel_slice_i32 s, keel_slice_cursor *c);
static inline i32 *keel_slice_i32_next(keel_slice_i32 s, keel_slice_cursor *c);
static inline keel_slice_i32 keel_slice_i32_partition(keel_slice_i32 s, size_t k, size_t w);
#endif /* KEEL_KEEL_SLICE_I32_PROTO_H */
