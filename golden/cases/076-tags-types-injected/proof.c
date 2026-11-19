/* proof.c — harness. `types` injects the tags set's own name bare (spec §4.1 regra 6),
   and its values resolve written at the set's own level, `Mode.LOW`, without the module's
   qualifier: `types` does not inject enum constants (regra 6), only the set's name. */

#include "keel.type.h"
#include "app/app_hue2.h"
#include "app/app_hue2_use.h"

int main(void) {
    if (app_hue2_use_level() != app_hue2_Mode_LOW) return 1;
    if (app_hue2_use_step(app_hue2_Mode_OFF) != app_hue2_Mode_LOW) return 2;
    if (app_hue2_use_step(app_hue2_Mode_HIGH) != app_hue2_Mode_HIGH) return 3;
    return 0;
}
