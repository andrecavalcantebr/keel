/* proof.c — harness. The walk over a list whose body braces are on their own lines
   (Allman), in the function and in the walk; the cursor is written qualified. */

#include "keel.type.h"
#include "app/app_wa.h"
#include <assert.h>

int main(void) {
    app_wa_Node c3 = { 3, NULL }, b2 = { 4, &c3 }, a1 = { 1, &b2 };
    app_wa_List l = { &a1 };
    assert(app_wa_sum(&l) == 8);

    app_wa_List empty = { NULL };
    assert(app_wa_sum(&empty) == 0);
    return 0;
}
