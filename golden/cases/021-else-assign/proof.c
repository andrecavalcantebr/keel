/* proof.c — harness for case 021: the `else` tail over an assignment, with
   the results declared outside the loop. NOT transpiler output. */
#include "app/app_reg.h"

static int eq(keel_slice_const_char s, const char *t, size_t n) {
    if (s.len != n) return 0;
    for (size_t i = 0; i < n; i++) if (s.ptr[i] != t[i]) return 0;
    return 1;
}

int main(void) {
    size_t k = app_reg_load(4);

    if (k != 3) return 1;                                   /* the fourth name is missing: `else break` */

    if (!eq(app_reg_people[0].name, "ana", 3)) return 2;
    if (!eq(app_reg_people[0].address, "rd one", 6)) return 3;
    if (app_reg_people[0].birth != 1.5) return 4;

    if (!eq(app_reg_people[1].address, "", 0)) return 5;    /* the assignment's default */
    if (app_reg_people[1].birth != 2.25) return 6;

    if (!eq(app_reg_people[2].name, "cid", 3)) return 7;
    if (app_reg_people[2].birth != 0.0) return 8;           /* default 0.0 */

    return 0;
}
