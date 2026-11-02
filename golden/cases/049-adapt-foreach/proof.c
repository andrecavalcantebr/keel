/* proof.c — harness. A pointer argument to a verb that takes its object by value gets `*`
   (language §4.4, rule 5); `foreach` takes a range through a pointer, and a container that is
   the result of a call, evaluated once. */

#include "keel.type.h"
#include "app/app_af.h"
#include "keel/keel_buffer_i32.h"
#include "keel/keel_range.h"
#include "keel/keel_slice_i32.h"

int main(void) {
    i32 a[3] = { 1, 2, 3 };
    keel_slice_i32 sl = { 3, a };
    keel_range rg = { 2, 6 };
    keel_buffer_i32 b = keel_buffer_i32_of(a, 3);
    if (app_af_adapt(&sl, &rg) != 11) return 1;       /* 3 + 2 + 6 */
    if (app_af_over_range(&rg) != 14) return 2;       /* 2 + 3 + 4 + 5 */
    if (app_af_over_call(&b) != 6) return 3;
    return 0;
}
