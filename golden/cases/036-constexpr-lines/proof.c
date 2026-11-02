/* proof.c — harness. K is 3 and the array is 7 long: the two multi-line initializers. */

#include "keel.type.h"
#include "app/app_cl.h"

int main(void) {
    return app_cl_f() == 10 && app_cl_K == 3 ? 0 : 1;
}
