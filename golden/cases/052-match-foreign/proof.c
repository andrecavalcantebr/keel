/* proof.c — harness. A `match` whose operand is a field of a struct that another module
   declares: the labels are that module's set, and the `switch` reads the tag of the field. */

#include "keel.type.h"
#include "app/lib/app_lib_other.h"
#include "app/app_fm.h"

int main(void) {
    app_lib_other_Gate g = { { app_lib_other_Phase_ON } };
    if (app_fm_foreign(&g) != 1) return 1;
    g.ph.tag = app_lib_other_Phase_OFF;
    if (app_fm_foreign(&g) != 2) return 2;
    return 0;
}
