#include "keel.type.h"
#include "keel/keel_outcome_i16.h"
#include "keel/keel_outcome.h"
#include "result.h"
#include "reading.h"
#include "span.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    keel_outcome_i16 a = result_accept(7);
    assert(keel_outcome_i16_ok(a) && keel_outcome_i16_value(a) == 7);
    a = result_accept(-7);
    assert(keel_outcome_i16_failed(a) && keel_outcome_i16_code(a) == -7);
    assert(result_get_value(7) == 7);
    assert(result_get_value(-7) == 0);

    /* span: the slice writes through to the array, and the range is half-open. */
    assert(span_sum() == 50);

    /* reading: the first byte of a file, and EOF when it cannot be opened. */
    FILE *f = tmpfile();
    assert(f != NULL);
    fputs("K", f);
    fflush(f);
    assert(reading_first("/nonexistent/keel-readme") == EOF);
    fclose(f);
    return 0;
}
