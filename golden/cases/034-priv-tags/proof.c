/* proof.c — harness. The set is private, so the proof sees only the number. */

#include "keel.type.h"
#include "pt.h"

int main(void) {
    return pt_trail() == 111 ? 0 : 1;
}
