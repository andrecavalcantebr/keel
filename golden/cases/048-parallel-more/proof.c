/* proof.c — harness. The policy written as a constexpr (two wins are enough), and a `parallel`
   whose header, body and closing brace are on one line. */

#include "keel.type.h"
#include "app/app_pm.h"
#include "keel/keel_buffer_i32.h"

int main(void) {
    i32 v[16];
    keel_buffer_i32 b = keel_buffer_i32_of(v, 16);
    for (size_t i = 0; i < 16; i++) keel_buffer_i32_set(&b, i, 1);
    if (app_pm_one_line(&b)) return 1;
    keel_buffer_i32_set(&b, 13, -7);
    if (!app_pm_one_line(&b)) return 2;
    if (app_pm_named_policy(&b)) return 3;          /* one winning part is not enough */
    keel_buffer_i32_set(&b, 1, -7);
    if (!app_pm_named_policy(&b)) return 4;         /* two parts win */
    return 0;
}
