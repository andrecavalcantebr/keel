/* proof.c — harness. With none of the macros defined, the compiler takes the last
   alternative of every group: the braces the cgen counted are the ones it pairs. */

#include "keel.type.h"
#include "app/app_cond.h"

int main(void) {
    if (app_cond_nested(2) != 1) return 1;
    if (app_cond_nested(0) != 0) return 2;
    if (app_cond_chain(4) != 3) return 3;
    if (app_cond_chain(2) != 2) return 4;
    if (app_cond_across(3) != 0) return 5;
    if (app_cond_balanced(7) != 7) return 6;
    if (app_cond_deep(1) != 2) return 7;
    if (app_cond_deep(0) != 0) return 8;
    return 0;
}
