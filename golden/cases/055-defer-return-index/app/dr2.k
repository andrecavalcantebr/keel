module app.dr2;
import keel.buffer as buffer types;

priv int trail = 0;
priv void note(int d) { trail = trail * 10 + d; }

pub void reset(void) { trail = 0; }
pub int trail_now(void) { return trail; }

/* One exit, literal index: the value is read before the cleanup, which writes
   another slot (§5.5, rule 5; §5.3, rule 6). */
pub int tail(void) {
    array i32 v[3] = {7, 8, 9};
    defer v[1] = 0;
    return v[0];
}

/* The cleanup writes the slot that `return` reads: the value is the one from
   before the cleanup, 7, and not 0. */
pub int head(void) {
    array i32 v[3] = {7, 8, 9};
    defer v[0] = 0;
    return v[0];
}

/* A variable index goes through keel_index (§5.3, rule 4). */
pub int pick(size_t i) {
    array i32 v[3] = {7, 8, 9};
    defer v[0] = 0;
    return v[i];
}

/* Two registrations, one exit: the cleanups run in reverse order, and the
   value is taken before them. */
pub int two(size_t i) {
    array i32 v[3] = {7, 8, 9};
    defer note(1);
    defer note(2);
    return v[i];
}

/* `break` inside `foreach`: the cleanup goes inline before the break and at
   the natural end of the body (§5.5.2, rules 1 and 2). */
pub size_t scan(buffer i32 *s) {
    size_t count = 0;
    foreach (i32 x, size_t i : s) {
        defer count++;
        if (x == 0) break;
    }
    return count;
}
