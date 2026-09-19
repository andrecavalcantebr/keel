/* proof.c — harness. The difference between `later` and `[now]` is SEMANTIC,
   and that is what this harness asserts: the same `defer` over the same code
   gives 3 and 1. */

#include "keel.type.h"
#include "app/app_dl.h"
#include <stdio.h>
int main(void) {
    app_dl_late();          if (app_dl_last() != 3) return 1;  /* value at the exit */
    app_dl_early();           if (app_dl_last() != 1) return 2;  /* valor no registro */
    app_dl_late_written();  if (app_dl_last() != 7) return 3;  /* `later` == the default */
    puts("ok");
    return 0;
}
