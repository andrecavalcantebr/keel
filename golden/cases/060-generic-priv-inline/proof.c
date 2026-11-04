/* proof.c — harness. It touches only the public interface of app.peek_use, and never peek:
   the private inline verb is reached through the public get of each instance. */

#include "app/app_peek_use.h"

int main(void) {
    if (app_peek_use_read_i32(7) != 7) return 1;
    if (app_peek_use_read_i32(-3) != -3) return 2;
    if (app_peek_use_read_i64(9) != 9) return 3;
    return 0;
}
