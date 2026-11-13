/* proof.c — harness. count is a function over IndexPtr (spec §4.14) declared in app.ibox_use,
   instantiated for app.ibox.box i32 (a concrete type of another generic module). Its body reaches
   the box verb length through the instance header; probe_count reads 3 back. */

#include "app/app_ibox_use.h"

int main(void) {
    if (app_ibox_use_probe_count() != 3) return 1;
    return 0;
}
