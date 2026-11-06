module g;

/* spec §2.4: the construction-level scanners that find the block a `defer`
   or a jump belongs to run AFTER the lexer's own k_check_delimiters has told
   every alternative of a group agrees, and must still see one block per
   group, not one per alternative — or a group between a jump and its target
   could hide a scope boundary from the check this proves it does not. */

/* a `goto` INTO a block, past a defer registered before the label, with a
   conditional group between the jump and the block it opens: refused the
   same way a plain block would be (jump-over-defer) */
pub int into(int x) {
    /* DIAG-NEXT: jump-over-defer */
    if (x) goto in;
#ifdef C
    {
#else
    {
#endif
        defer g2();
in:
        x++;
    }
    return x;
}
