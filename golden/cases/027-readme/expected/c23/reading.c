/* reading.c — generated from reading.k by cgen, C23 profile. */
#include "reading.h"
#line 1 "reading.k"



int reading_first(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) return EOF;

    { int keel__rv0 = fgetc(file);
#line 7 "reading.k"
    fclose(file);
    return keel__rv0; }
}
