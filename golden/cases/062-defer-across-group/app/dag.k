module app.dag;
import_c <stdio.h>;

/* spec §2.4, carried into the scanners that pair braces and parens after
   k_check_delimiters has already told every alternative of a conditional
   group agrees (isl_enclosing_brace, the checker's block_end,
   isl_close_paren/isl_close_bracket in parser_islands*.c, and defer's own
   block_open/block_close in emit/defer.c): a group between a construction
   and the boundary it needs counts as ONE block, not one per alternative.
   Nothing here defines A, B or D, so the compiler takes the last
   alternative of each group, and the proof reads the result. */

/* a `defer` registered in the function's own scope, with a conditional group
   further on — opening a nested `if`, itself holding another one — before
   the `goto` it has to reach past: isl_enclosing_brace (backward, from the
   defer) and block_end/block_close (forward, past the whole group to the
   function's own closing `}`) must resolve to the one real block each side
   of the group shares. Before this was fixed, this refused with
   implementation-limit: the group's three textual `{` (two nested
   alternatives) counted as three blocks, and the real closing `}` was never
   found. */
pub int crosses(int x) {
    defer printf("cleanup %d\n", x);
    if (x) {
#ifdef A
#  ifdef B
        if (x) {
#  else
        if (!x) {
#  endif
#else
        if (x > 1) {
#endif
            x++;
        }
        if (x) goto out;
    }
out:
    return x;
}

/* a call whose argument list is wrapped by a group: isl_close_paren counts
   only the call's own two parens, not the group's braces (irrelevant to it,
   and absent from either alternative here) */
int h(int a, int b) { return a + b; }
pub int wrapped(int x) {
    return h(
#ifdef D
        x, 1
#else
        x, 2
#endif
    );
}
