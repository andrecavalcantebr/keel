/* proof.c — harness of case 055. NOT transpiler output.
 *
 * The value of `return` is read BEFORE the cleanups (backend §5.5, rule 5), and
 * the cleanups run in reverse order of registration (§5.5, rule 1). Each check
 * below fails with its own code, so the first wrong order is the one reported. */
#include "keel.type.h"
#include "app/app_dr2.h"
#include "keel/keel_buffer_i32.h"
#include <stdio.h>
int main(void) {
    /* literal index; the cleanup writes another slot */
    if (app_dr2_tail() != 7) return 1;

    /* the cleanup writes the slot that `return` reads: 7, not 0 */
    if (app_dr2_head() != 7) return 2;

    /* variable index, through keel_index; the value is still the pre-cleanup one */
    if (app_dr2_pick(0) != 7) return 3;
    if (app_dr2_pick(2) != 9) return 4;

    /* two registrations, one exit: note(2) then note(1), so (0*10+2)*10+1 */
    app_dr2_reset();
    if (app_dr2_two(1) != 8) return 5;
    if (app_dr2_trail_now() != 21) return 6;

    /* break: the cleanup of the iteration that breaks runs, and so does the one
       that ends naturally before it (count 1 for 5, then 2 for the 0 that breaks) */
    i32 a[3] = {5, 0, 7};
    keel_buffer_i32 ba = keel_buffer_i32_of(a, 3);
    if (app_dr2_scan(&ba) != 2) return 7;

    /* no break: every iteration ends naturally, so every one cleans up */
    i32 c[3] = {5, 6, 7};
    keel_buffer_i32 bc = keel_buffer_i32_of(c, 3);
    if (app_dr2_scan(&bc) != 3) return 8;

    /* break on the first element: only its cleanup ran */
    i32 e[2] = {0, 1};
    keel_buffer_i32 be = keel_buffer_i32_of(e, 2);
    if (app_dr2_scan(&be) != 1) return 9;
    puts("ok");
    return 0;
}
