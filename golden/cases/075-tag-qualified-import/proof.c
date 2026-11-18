/* proof.c — harness. `alias.Set.VALUE` from another module reads the value of the set that module
   declares: the island covers the five tokens and writes `app_hue_Hue_GREEN`. */

#include "keel.type.h"
#include "app/app_hue.h"
#include "app/app_hue_use.h"

int main(void) {
    if (app_hue_use_green() != 5) return 1;
    if (app_hue_use_next(app_hue_Hue_RED) != app_hue_Hue_GREEN) return 2;
    if (app_hue_use_next(app_hue_Hue_GREEN) != app_hue_Hue_BLUE) return 3;
    return 0;
}
