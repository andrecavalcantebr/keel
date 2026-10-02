module d;
pub int f(int x) {
#if X
    if (x) {
#else
    if (!x) {
#endif
        return 1;
    }
    return (x];                              /* DIAG: unmatched-delimiter */
}
pub int g(int x) {
    /* DIAG-NEXT: delimiter-mismatch-across-branches */
#ifdef Y
    x++; {
#endif
    }
    return x;
}
