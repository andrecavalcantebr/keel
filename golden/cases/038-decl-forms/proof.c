/* proof.c — harness. Every declaration form of dc.k is used once in total(10):
   10 + 5 + 3 + 4 + 1 + 2 + 2 = 27, and the two variables are the module's. */

#include "keel.type.h"
#include "app/app_dc.h"

int main(void) {
    struct app_dc_Box b = { 1 };
    union app_dc_Num u = { 2 };
    app_dc_PP pp = 0;
    (void)b; (void)u; (void)pp;
    if (app_dc_total(10) != 27) return 1;
    if (app_dc_x != 1 || app_dc_y != 2) return 2;
    return 0;
}
