/* keel/keel_range.type.h — o `.h` do módulo keel.range.
   `range` NÃO é `byref`: os verbos recebem cópia (linguagem §4.11). Declara os
   quatro que fazem dele contável e percorrível: first, limit, length, get. */
#ifndef KEEL_KEEL_RANGE_TYPE_H
#define KEEL_KEEL_RANGE_TYPE_H
#include "keel.type.h"

typedef struct keel_range { size_t first; size_t limit; } keel_range;
#endif /* KEEL_KEEL_RANGE_TYPE_H */
