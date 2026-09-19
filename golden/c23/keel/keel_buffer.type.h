/* keel/keel_buffer.type.h — o que o módulo `keel.buffer` declara uma vez, e não por
   instância: o cursor guarda uma posição e não menciona `T` (backend §5.11). */
#ifndef KEEL_KEEL_BUFFER_TYPE_H
#define KEEL_KEEL_BUFFER_TYPE_H
#include "keel.type.h"

typedef struct keel_buffer_cursor { size_t i; } keel_buffer_cursor;
#endif /* KEEL_KEEL_BUFFER_TYPE_H */
