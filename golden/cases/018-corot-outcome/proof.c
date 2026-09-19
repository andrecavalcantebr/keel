#include "keel.type.h"
#include "keel/keel_corot.h"
#include "keel/keel_outcome_i32.h"
#include "keel/keel_outcome_void.h"
#include "keel/keel_outcome.h"
#include "results.h"
#include <assert.h>
int main(void) {
    keel_corot r = results_state(-7);
    assert(keel_corot_ok(r));
    assert(!keel_corot_ongoing(r) && !keel_corot_faulted(r));
    r = results_state(0);
    assert(keel_corot_ongoing(r));
    assert(!keel_corot_ok(r) && !keel_corot_faulted(r));
    r = results_state(7);
    assert(keel_corot_faulted(r) && keel_corot_code(r) == 7);
    assert(!keel_corot_ok(r) && !keel_corot_ongoing(r));
    assert(!results_is_fault(-7) && !results_is_fault(0) && results_is_fault(7));
    assert(keel_outcome_void_failed(results_final(true, -7)));
    assert(keel_outcome_void_failed(results_final(true, 7)));
    assert(keel_outcome_void_ok(results_final(false, 0)));
    assert(results_repair(-7) == 5 && results_repair(7) == 5);
    /* The verb modifies the receiver even when its returned copy is ignored. */
    keel_outcome_i32 result = {7, 12};
    keel_outcome_i32_win(&result);
    assert(keel_outcome_i32_ok(result));
    assert(keel_outcome_i32_value(result) == 12);
    keel_outcome_i32 copy = keel_outcome_i32_fail(&result, -9);
    assert(keel_outcome_i32_code(result) == -9 && copy.code == -9);
    assert(result.value == 12 && copy.value == 12);
    keel_outcome_i32_value1(&result, 42);
    assert(result.code == -9 && result.value == 42);
    copy = keel_outcome_i32_none(&result);
    assert(result.code == keel_outcome_NONE && copy.code == keel_outcome_NONE);
    assert(result.value == 42 && copy.value == 42);
    assert(keel_outcome_i32_failed(result) && !keel_outcome_i32_ok(result));
    assert(keel_outcome_i32_code(result) == keel_outcome_NONE);
    assert(result.code == keel_outcome_NONE && result.value == 42);
    keel_outcome_i32 objects[2] = {{-1, 0}, {-2, 0}};
    size_t index = 0;
    i32 valor = 80;
    copy = keel_outcome_i32_win1(&objects[index++], valor++);
    assert(index == 1 && valor == 81);
    assert(objects[0].code == 0 && objects[0].value == 80);
    assert(copy.code == 0 && copy.value == 80 && objects[1].code == -2);
    keel_outcome_void no_value = {0};
    keel_outcome_void absence = keel_outcome_void_none(&no_value);
    assert(keel_outcome_void_failed(no_value));
    assert(keel_outcome_void_code(absence) == keel_outcome_NONE);
    keel_outcome_void_win(&no_value);
    assert(keel_outcome_void_ok(no_value));
    assert(keel_outcome_void_failed(absence));
    i32 counter = 0;
    r = results_evaluate_cleanup(&counter);
    assert(keel_corot_code(r) == 1 && counter == 2);
    return 0;
}
