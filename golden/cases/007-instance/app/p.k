module app.p;
import coll types;
import_c <stdio.h>;

pub int main(int argc, char **argv) {
    array i32 v[4];
    stack i32 s;
    s.cap = 4; s.len = 0; s.ptr = v;

    if (length(s) != 0) return 1;
    if (!push(s, 7)) return 2;              /* body lives in instances.c */
    if (length(s) != 1) return 3;
    if (coll.EMPTY != -1) return 4;           /* bare name, without the argument */
    puts("ok");
    return 0;
}
