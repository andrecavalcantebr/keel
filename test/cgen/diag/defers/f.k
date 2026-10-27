module f;
import_c <stdio.h>;

defer puts("x");                             /* DIAG: defer-at-file-scope */

pub int sw(int k) {
    switch (k) {
    case 0: {
        defer { puts("a"); }
        break;
    }
    case 1:
        puts("b");
        defer { puts("c"); }                 /* DIAG: defer-in-control-block */
        break;
    case 2:                                  /* DIAG: jump-over-defer */
        break;
    }
    return 0;
}

pub int jumps(int k) {
    if (k) goto inside;                      /* DIAG: jump-over-defer */
    {
        defer { puts("d"); }
inside:
        puts("e");
    }
    return 0;
}

pub int shadow(int fd) {
    defer { printf("%d", fd); }
    {
        int fd = 3;
        if (fd) return 1;                    /* DIAG: defer-later-shadowed */
    }
    return 0;
}
