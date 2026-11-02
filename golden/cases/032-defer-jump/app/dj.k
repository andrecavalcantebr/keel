module app.dj;

/* The order of the cleanups is the whole proof (spec §4.6, rules 1 and 3):
   each one appends a letter, and proof.c reads the trail back. */
priv char trail[8] = {0};
priv size_t walked = 0;

priv void note(char c) { if (walked < 8) trail[walked++] = c; }

pub void reset(void) { walked = 0; }
pub size_t walked_now(void) { return walked; }
pub char step(size_t i) { return i < walked ? trail[i] : 0; }

/* `goto` out of a scope is an exit like any other (backend §5.5, rule 1), and a
   `goto` exit puts every cleanup inline (§5.5.2, rules 1 and 2). */
pub int jumped(int k) {
    {
        defer note('g');
        if (k == 0) goto out;
        note('b');
    }
    return 0;
out:
    return -1;
}

/* A `defer` of an inner block goes inline, and the ladder is the function's
   (§5.5.2, rule 6). The third one is registered after the two returns, so they
   do not run it (§5.5, rule 3). */
pub int nested(int k) {
    defer note('1');
    {
        defer note('2');
        if (k == 0) return 1;
        if (k == 1) return 2;
        defer note('3');
    }
    return 3;
}
