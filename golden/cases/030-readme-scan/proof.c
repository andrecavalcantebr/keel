/* proof.c — harness. Each worker gets one part of the container and writes
   its sum at its own index; nothing else is shared. */

#include "keel.type.h"
#include "keel/keel_buffer_i32.h"
#include "keel/keel_buffer_u32.h"
#include "scan.h"
#include <assert.h>
int main(void) {
    i32 v[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    u32 t[4] = { 0, 0, 0, 0 };
    keel_buffer_i32 xs = keel_buffer_i32_of(v, 8);
    keel_buffer_u32 totals = keel_buffer_u32_of(t, 4);
    scan_sum(&xs, &totals);
    assert(t[0] == 3 && t[1] == 7 && t[2] == 11 && t[3] == 15);

    /* fewer elements than workers: the parts that get nothing sum to zero */
    i32 w[2] = { 5, 6 };
    u32 t2[4] = { 9, 9, 9, 9 };
    keel_buffer_i32 ys = keel_buffer_i32_of(w, 2);
    keel_buffer_u32 totals2 = keel_buffer_u32_of(t2, 4);
    scan_sum(&ys, &totals2);
    assert(t2[0] == 5 && t2[1] == 6 && t2[2] == 0 && t2[3] == 0);
    return 0;
}
