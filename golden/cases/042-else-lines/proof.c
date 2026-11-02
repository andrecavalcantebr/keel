/* proof.c — harness. The `else` clause keeps the line break the author wrote: the statement,
   or the block, is on the line after it, and the exit still runs only on failure. */

#include "keel.type.h"
#include "app/app_es.h"

int main(void) {
    if (app_es_split(5) != 5 || app_es_split(-1) != -1) return 1;
    if (app_es_split_block(3) != 3 || app_es_split_block(-1) != -2) return 2;
    return 0;
}
