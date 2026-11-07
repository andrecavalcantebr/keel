/* proof.c — harness. view(box) exposes box's storage as a read-only slice: the
   values it reads match what was stored, through an ordinary keel.slice. */

#include "app/app_cbox_use.h"

int main(void) {
    if (app_cbox_use_probe_length() != 3) return 1;
    if (app_cbox_use_probe_get(0) != 10) return 2;
    if (app_cbox_use_probe_get(1) != 20) return 3;
    if (app_cbox_use_probe_get(2) != 30) return 4;
    return 0;
}
