/* proof.c — harness. A modifier declared over several lines, one with a comment inside, and a
   private function that the public one calls: the body of both is in inst.c, and the private
   one is static there. */

#include "keel.type.h"
#include "gx_stack_i32.h"
#include "gx_cell_i32.h"

int main(void) {
    gx_cell_i32 c = { 7 };
    gx_stack_i32 s = { 4, 2, 0 };
    if (gx_cell_i32_get(&c) != 7) return 1;
    if (gx_stack_i32_length(&s) != 2) return 2;
    return 0;
}
