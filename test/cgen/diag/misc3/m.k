module m;
import keel.buffer as buffer types;
import lib.st as st;
import_c <stdio.h>;

pub int f(int k) {
    int a = st.LIMIT;
    int b = st.IDLE;                             /* DIAG: enum-constant-without-type */
    defer [now int arr[3]] { (void)arr; }        /* DIAG: hidden-declarator */
    defer [now int x, char *s] { puts(s); }
    return a + b + k;
}

pub int (*table(void))[4] {                      /* DIAG: hidden-declarator */
    defer { puts("t"); }
    return 0;
}

pub void g(i32 *p) {
    buffer i32 b = buffer.from(p, 4);
    b = buffer.from(p, 8);
    buffer.length(buffer.from(p, 2));            /* DIAG: from-without-target */
}

pub buffer i32 h(i32 *p) { return buffer.from(p, 4); }
