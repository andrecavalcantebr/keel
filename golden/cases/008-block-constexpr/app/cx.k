module app.cx;
import_c <stdio.h>;

pub typedef struct { int N; int MAX; } Cfg;

pub constexpr size_t MAX = 4;          /* file scope: carries the module prefix */

/* Block scope: under C11 the user's name would become a macro and capture
   `c->N`. That is why the backend generates a name of its own and rewrites
   the uses (§9.2). */
pub int sum(Cfg *c) {
    constexpr size_t N = 8;
    array i32 v[N];
    int t = 0;
    foreach (auto i : 0..N) { v[i] = (i32)i; t += v[i]; }
    c->N   = (int)N;                   /* member: NOT rewritten */
    c->MAX = (int)MAX;                 /* likewise */
    return t + (int)sizeof(v) / (int)sizeof(i32);
}

/* Two sibling blocks with the same name: the ordinal separates them. */
pub int other(void) {
    { constexpr size_t N = 2; array i32 a[N]; a[0] = 1; return (int)(sizeof(a)/sizeof(i32)); }
}

pub int main(int argc, char **argv) {
    Cfg c = {0};
    if (sum(&c) != 28 + 8) return 1;
    if (c.N != 8 || c.MAX != 4) return 2;
    if (other() != 2) return 3;
    puts("ok");
    return 0;
}
