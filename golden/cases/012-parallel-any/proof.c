/* proof.c — harness. Asserts what §4.8 promises and nothing more.
   The recorded index is the PART's, not the container's: whoever partitions
   gets a view, and converting to a global index is the program's business. */

#include "keel.type.h"
#include "app/app_bs.h"
#include "keel/keel_buffer_i32.h"
#include "keel/keel_buffer_size_t.h"
#include <stdio.h>
int main(void) {
    i32 v[64]; size_t o[4] = {0,0,0,0};
    for (size_t i = 0; i < 64; i++) v[i] = (i32)i;
    keel_buffer_i32    b  = keel_buffer_i32_of(v, 64);
    keel_buffer_size_t ob = keel_buffer_size_t_of(o, 4);

    /* target 40: part 2 = [32,48), local position 8 → records 9 */
    if (!app_bs_found_any(&b, 40, &ob)) return 1;
    if (o[2] != 9) return 2;
    if (o[0] || o[1] || o[3]) return 3;

    /* target absent: nobody wins, and without a win the `ANY` policy is not
       satisfied — that is what tells finishing from finding. */
    size_t o2[4] = {0,0,0,0};
    keel_buffer_size_t ob2 = keel_buffer_size_t_of(o2, 4);
    if (app_bs_found_any(&b, 999, &ob2)) return 4;
    puts("ok");
    return 0;
}
