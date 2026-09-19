/* keel/keel_slice_const_char.h — gerado de keel/slice.k pelo cgen, perfil C23. */
#ifndef KEEL_KEEL_SLICE_CONST_CHAR_H
#define KEEL_KEEL_SLICE_CONST_CHAR_H
#include "keel/keel_slice_const_char.type.h"

static inline keel_slice_const_char keel_slice_const_char_from(const char *p, size_t n);
static inline size_t keel_slice_const_char_length(keel_slice_const_char s);

static inline keel_slice_const_char keel_slice_const_char_from(const char *p, size_t n) { return (keel_slice_const_char){ p ? n : 0, p }; }
static inline size_t keel_slice_const_char_length(keel_slice_const_char s) { return s.len; }
#endif /* KEEL_KEEL_SLICE_CONST_CHAR_H */
