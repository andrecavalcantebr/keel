#include "keel.type.h"
#include "keel/keel_outcome_i16.h"
#include "keel/keel_outcome.h"
#include "result.h"
#include <assert.h>
int main(void) {
    keel_outcome_i16 a = result_accept(7);
    assert(keel_outcome_i16_ok(a) && keel_outcome_i16_value(a) == 7);
    a = result_accept(-7);
    assert(keel_outcome_i16_failed(a) && keel_outcome_i16_code(a) == -7);
    assert(result_get_value(7) == 7);
    assert(result_get_value(-7) == 0);
    return 0;
}
