/* proof.c — harness. Three more shapes of `match` arms: a statement with a comment on the label's
   line, two labels stacked on one line, and the whole match on one line. Each takes the arm A
   (the zero tag) and then B. */

#include "keel.type.h"
#include "app/app_mm.h"

int main(void) {
    if (app_mm_comment_arm() != 1) return 1;
    if (app_mm_stacked() != 1) return 2;
    if (app_mm_oneline() != 1) return 3;
    return 0;
}
