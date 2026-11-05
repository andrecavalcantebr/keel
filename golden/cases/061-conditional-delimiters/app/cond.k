module app.cond;

/* Conditional groups and the braces they leave open (spec §2.4). Each group is
   counted alternative by alternative, from the same entry, and every
   alternative leaves the same opening behind; the group is then resolved once.
   Nothing here defines A, B, X, Z, W or L0..L7, so the compiler takes the
   last alternative of each group, and the proof reads the result. */

/* nested groups, resolved inside out */
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

/* a chain of alternatives: `#elif`, and `#  else` spaced (`#elifdef` is C23 only: the diag test has it) */
pub int chain(int x) {
#if X == 1
    if (x == 1) {
#elif X == 2
    if (x == 2) {
#elif defined(Z)
    if (x == 3) {
#elif !defined(W)
    if (x == 4) {
#  else
    if (x == 5) {
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

/* one opening of the same kind left behind, closed after the group */
pub int balanced(int x) {
#if X
    return (x
#else
    return ((x)
#endif
    );
}

/* groups nested eight deep, each with its own #else and #endif */
pub int deep(int x) {
#ifdef L0
#ifdef L1
#ifdef L2
#ifdef L3
#ifdef L4
#ifdef L5
#ifdef L6
#ifdef L7
    if (x) {
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
