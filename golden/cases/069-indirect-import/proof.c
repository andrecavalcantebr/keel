/* proof.c — harness. ia imports only ib; ib imports ic. ia uses ic.value()
   without importing ic directly — keel-spec §4.1 regra 8: resolution
   reaches it through the transitive closure, with indirect-import (info),
   not an error. */

#include "ia.h"

int main(void) {
    if (ia_use_it() != 42) return 1;
    return 0;
}
