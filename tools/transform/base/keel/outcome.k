/*
 * module keel.outcome type $T
 *
 * DESC: keel generic outcome value for parser keel
 * AUTHOR: andre cavalcante
 * DATE: Sep, 2026
 * LICENSE: GPL v3
 */

%type
#include "keel.type.h"
$A

typedef struct keel_outcome_$T {
    i32 code;
    $T value;
} keel_outcome_$T;

%impl
// OK e NONE sao do modulo, nao da instancia, e sao L1: chegam pelo keel.type.h

static inline keel_outcome_$T keel_outcome_$T_win(keel_outcome_$T *r) {
    r->code = keel_outcome_OK;
    return *r;
}

static inline keel_outcome_$T keel_outcome_$T_win_1(keel_outcome_$T *r, $T value) {
    r->code = keel_outcome_OK;
    r->value = value;
    return *r;
}

static inline keel_outcome_$T keel_outcome_$T_fail(keel_outcome_$T *r, i32 code) {
    r->code = code;
    return *r;
}

static inline keel_outcome_$T keel_outcome_$T_none(keel_outcome_$T *r) {
    r->code = keel_outcome_NONE;
    return *r;
}

static inline bool keel_outcome_$T_ok(const keel_outcome_$T *r) {
    return r->code == keel_outcome_OK;
}

static inline bool keel_outcome_$T_failed(const keel_outcome_$T *r) {
    return r->code != keel_outcome_OK;
}

static inline i32 keel_outcome_$T_code(const keel_outcome_$T *r) {
    return r->code;
}

static inline $T keel_outcome_$T_value(const keel_outcome_$T *r) {
    return r->value;
}

static inline void keel_outcome_$T_value_1(keel_outcome_$T *r, $T value) {
    r->value = value;
}
