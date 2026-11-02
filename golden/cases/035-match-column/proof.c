/* proof.c — harness. The column is by pointer, so the proof lends it an array. */

#include "keel.type.h"
#include "app/app_nd.h"

int main(void) {
    app_nd_Kind kinds[2] = { app_nd_Kind_ADD, app_nd_Kind_LIT };
    struct app_nd_nodes t = { .kind = kinds, .n = 2, .cap = 2 };
    if (app_nd_weight(&t, 0) != 2) return 1;
    if (app_nd_weight(&t, 1) != 1) return 2;
    return 0;
}
