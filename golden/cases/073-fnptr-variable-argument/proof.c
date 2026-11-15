/* proof.c — harness. `run` passes the function `hash` to `forward`, which passes its own
   parameter `h` (a function-pointer variable) on to `apply`: the argument must be the
   pointer itself, not its address. */
#include "keel.type.h"
#include "fnptr_arg.h"

int main(void) {
    if (fnptr_arg_run() != 3) return 1;
    return 0;
}
