/* proof.c — harness. Two slices of an array (3 + 3 elements) and the lengths of the two
   module arrays declared together (3 + 2): 11. */

#include "keel.type.h"
#include "app/app_rg.h"

int main(void) {
    return app_rg_window() == 11 ? 0 : 1;
}
