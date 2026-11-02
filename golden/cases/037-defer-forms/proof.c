/* proof.c — harness. The cleanups run in the order they are written, after the
   body, whatever the shape of their writing. */

#include "keel.type.h"
#include "app/app_df.h"

int main(void) {
    app_df_reset(); app_df_braced();
    if (app_df_result() != 312) return 1;       /* add(3), then add(1), add(2) */
    app_df_reset(); app_df_spread();
    if (app_df_result() != 54) return 2;        /* add(5), then add(4) */
    app_df_indexed();
    if (app_df_slot(1) != 9) return 3;          /* slots[1] = 2, then = 9 */
    app_df_reset();
    if (app_df_returned() != 0) return 4;       /* the value is read before the cleanup */
    if (app_df_result() != 6) return 5;
    return 0;
}
