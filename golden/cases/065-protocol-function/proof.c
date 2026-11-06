/* proof.c — harness. `span_of` is a plain, non-inline function that calls
   `span`, a function over a protocol (spec §4.14) declared in `pf` itself,
   outside any `--instance` — the case the backend's own `slice.of` example
   never exercises, since keel's base always calls it through `--instance`. */
#include "keel.type.h"
#include "pf.h"

int main(void) {
    pf_box b = { 3, 10 };
    if (pf_span_of(b) != 7) return 1;   /* (10 - 3) */
    return 0;
}
