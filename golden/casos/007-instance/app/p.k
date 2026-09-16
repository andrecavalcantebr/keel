module app.p;
import pilha types;
import_c <stdio.h>;

pub int main(int argc, char **argv) {
    array i32 v[4];
    stack i32 s;
    s.cap = 4; s.len = 0; s.ptr = v;

    if (length(s) != 0) return 1;
    if (!empurra(s, 7)) return 2;              /* corpo vive em instancias.c */
    if (length(s) != 1) return 3;
    if (pilha.VAZIA != -1) return 4;           /* nome nu, sem o argumento */
    puts("ok");
    return 0;
}
