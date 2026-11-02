module app.d10;
import_c <stdlib.h>;
pub int f(int n) {
    int *p = malloc(sizeof(int));
    defer free(p); // c
    defer {
        n++; // inside
    }
    return n;
}
