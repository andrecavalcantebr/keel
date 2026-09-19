/* keel/keel_buffer_i32.type.h — instância de `buffer i32` (backend §5.2, §5.13). */
#ifndef KEEL_KEEL_BUFFER_I32_TYPE_H
#define KEEL_KEEL_BUFFER_I32_TYPE_H
#include "keel.type.h"

typedef struct keel_buffer_i32 { size_t cap; size_t len; i32 *ptr; } keel_buffer_i32;
#endif /* KEEL_KEEL_BUFFER_I32_TYPE_H */
