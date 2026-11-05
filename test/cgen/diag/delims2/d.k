module d2;

/* spec §2.4. The alternatives of a conditional group are counted one by one,
   from the same entry, and must leave the same openings behind. Everything
   here is clean unless a line says otherwise. */

/* nested groups, every alternative of both opens one `{`: resolved inside out */
pub int nested(int x) {
#ifdef A
#  ifdef B
    if (x) {
#  else
    if (!x) {
#  endif
#else
    if (x > 1) {
#endif
        return 1;
    }
    return 0;
}

/* a chain: `#elif`, `#elifdef`, `#elifndef` and `#else` are alternatives of the
   same group, and the spacing of `#  else` does not change what it is */
pub int chain(int x) {
#if X == 1
    while (x) {
#elif X == 2
    for (; x; ) {
#elifdef Z
    if (x) {
#elifndef W
    do {
#  else
    switch (x) { default:
#endif
        x--;
    }
    return x;
}

/* the group opens a block that ends after it */
pub int across(int x) {
#if X
    if (x) {
#else
    while (x) {
#endif
        x--;
    }
    return x;
}

/* the same delta in every alternative: one opening of the same kind left behind */
pub int balanced(int x) {
#if X
    return (x
#else
    return ((x)
#endif
    );
}

/* an opening under `#ifdef` alone: the missing `#else` is an empty alternative */
pub int lonely(int x) {
    /* DIAG-NEXT: delimiter-mismatch-across-branches */
#ifdef Y
    if (x) {
#endif
        x++;
    }
    return x;
}

/* `#elif` chain without `#else`: the last alternative is empty */
pub int chained_lonely(int x) {
    /* DIAG-NEXT: delimiter-mismatch-across-branches */
#if X
    if (x) {
#elif Y
    while (x) {
#endif
        x++;
    }
    return x;
}

/* only the inner group diverges: one diagnostic, at the inner `#if` */
pub int inner(int x) {
#ifdef A
    /* DIAG-NEXT: delimiter-mismatch-across-branches */
#  ifdef B
    if (x) {
#  endif
    x++;
}
#else
    x--;
#endif
    return x;
}

/* the inner group agrees, the outer does not: one diagnostic, at the outer */
pub int outer(int x) {
    /* DIAG-NEXT: delimiter-mismatch-across-branches */
#ifdef A
#  ifdef B
    if (x) {
#  else
    if (!x) {
#  endif
#endif
        x++;
    }
    return x;
}

/* kinds count, not only the number: `{` against `(` is a disagreement */
pub int kinds(int x) {
    /* DIAG-NEXT: delimiter-mismatch-across-branches */
#if X
    if (x) {
#else
    if ((x)) (
#endif
        x++;
    }
    return x;
}

/* what is not a delimiter of the program is not counted */
#define OPEN (
#define BRACES { }
#define SKIP(x) ((x) + [0)
pub const char *quoted(void) {
    /* an opening in a comment: { ( [ */
    const char *s = "{ ( [ ";
    char c = '{';
    (void)c;
    return s;
}

/* 63 levels, what C requires of a compiler: each #else and #endif pairs with its own #ifdef */
pub int deep(int x) {
#ifdef L0
#ifdef L1
#ifdef L2
#ifdef L3
#ifdef L4
#ifdef L5
#ifdef L6
#ifdef L7
#ifdef L8
#ifdef L9
#ifdef L10
#ifdef L11
#ifdef L12
#ifdef L13
#ifdef L14
#ifdef L15
#ifdef L16
#ifdef L17
#ifdef L18
#ifdef L19
#ifdef L20
#ifdef L21
#ifdef L22
#ifdef L23
#ifdef L24
#ifdef L25
#ifdef L26
#ifdef L27
#ifdef L28
#ifdef L29
#ifdef L30
#ifdef L31
#ifdef L32
#ifdef L33
#ifdef L34
#ifdef L35
#ifdef L36
#ifdef L37
#ifdef L38
#ifdef L39
#ifdef L40
#ifdef L41
#ifdef L42
#ifdef L43
#ifdef L44
#ifdef L45
#ifdef L46
#ifdef L47
#ifdef L48
#ifdef L49
#ifdef L50
#ifdef L51
#ifdef L52
#ifdef L53
#ifdef L54
#ifdef L55
#ifdef L56
#ifdef L57
#ifdef L58
#ifdef L59
#ifdef L60
#ifdef L61
#ifdef L62
    if (x) {
#else
    if (x > 62) {
#endif
#else
    if (x > 61) {
#endif
#else
    if (x > 60) {
#endif
#else
    if (x > 59) {
#endif
#else
    if (x > 58) {
#endif
#else
    if (x > 57) {
#endif
#else
    if (x > 56) {
#endif
#else
    if (x > 55) {
#endif
#else
    if (x > 54) {
#endif
#else
    if (x > 53) {
#endif
#else
    if (x > 52) {
#endif
#else
    if (x > 51) {
#endif
#else
    if (x > 50) {
#endif
#else
    if (x > 49) {
#endif
#else
    if (x > 48) {
#endif
#else
    if (x > 47) {
#endif
#else
    if (x > 46) {
#endif
#else
    if (x > 45) {
#endif
#else
    if (x > 44) {
#endif
#else
    if (x > 43) {
#endif
#else
    if (x > 42) {
#endif
#else
    if (x > 41) {
#endif
#else
    if (x > 40) {
#endif
#else
    if (x > 39) {
#endif
#else
    if (x > 38) {
#endif
#else
    if (x > 37) {
#endif
#else
    if (x > 36) {
#endif
#else
    if (x > 35) {
#endif
#else
    if (x > 34) {
#endif
#else
    if (x > 33) {
#endif
#else
    if (x > 32) {
#endif
#else
    if (x > 31) {
#endif
#else
    if (x > 30) {
#endif
#else
    if (x > 29) {
#endif
#else
    if (x > 28) {
#endif
#else
    if (x > 27) {
#endif
#else
    if (x > 26) {
#endif
#else
    if (x > 25) {
#endif
#else
    if (x > 24) {
#endif
#else
    if (x > 23) {
#endif
#else
    if (x > 22) {
#endif
#else
    if (x > 21) {
#endif
#else
    if (x > 20) {
#endif
#else
    if (x > 19) {
#endif
#else
    if (x > 18) {
#endif
#else
    if (x > 17) {
#endif
#else
    if (x > 16) {
#endif
#else
    if (x > 15) {
#endif
#else
    if (x > 14) {
#endif
#else
    if (x > 13) {
#endif
#else
    if (x > 12) {
#endif
#else
    if (x > 11) {
#endif
#else
    if (x > 10) {
#endif
#else
    if (x > 9) {
#endif
#else
    if (x > 8) {
#endif
#else
    if (x > 7) {
#endif
#else
    if (x > 6) {
#endif
#else
    if (x > 5) {
#endif
#else
    if (x > 4) {
#endif
#else
    if (x > 3) {
#endif
#else
    if (x > 2) {
#endif
#else
    if (x > 1) {
#endif
#else
    if (x > 0) {
#endif
        x++;
    }
    return x;
}
