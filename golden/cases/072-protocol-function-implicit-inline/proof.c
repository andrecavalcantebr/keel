/* proof.c — harness. `span_of` is a plain, non-inline function that calls
   `span`, a function over a protocol (spec §4.14) declared in `pf2` itself,
   outside any `--instance`. `span` is written without `inline`: rule 8 makes
   it inline anyway, so the link must still resolve. */
#include "keel.type.h"
#include "pf2.h"

int main(void) {
    pf2_box b = { 3, 10 };
    if (pf2_span_of(b) != 7) return 1;   /* (10 - 3) */
    return 0;
}
