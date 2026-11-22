/* proof.c — harness. A table of 120 rows, each naming a tag: its size must not bound the
   generation (a limit on edits per declaration refused it from 30 rows on). The .h
   declares it without the initializer; the .c holds the 120 rows with the tags renamed. */

#include "keel.type.h"
#include "app/app_tbl.h"

int main(void) {
    for (int i = 0; i < 120; i++) {
        app_tbl_Op want = i % 3 == 0 ? app_tbl_Op_ADD : i % 3 == 1 ? app_tbl_Op_SUB : app_tbl_Op_NOP;
        if (app_tbl_table[i].op != want) return 1;
        if (app_tbl_table[i].arg != i) return 2;
    }
    return 0;
}
