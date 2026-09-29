module c;
import keel.buffer as buffer types;
import keel.slice as slice types;
import_c <stdio.h>;

pub size_t f(buffer i32 b, i32 n, slice i32 s) {
    size_t x = buffer.length(&b);                /* DIAG: address-in-object-position */
    x += buffer.length(fopen("x", "r"));         /* DIAG: not-a-container-expression */
    x += buffer.length(n);                       /* DIAG: not-a-container-expression */
    x += buffer.length(s);                       /* DIAG: wrong-qualifier */
    x += buffer.length(b + 1);                   /* DIAG: not-a-container-expression */
    x += buffer.length(b);
    return x;
}
