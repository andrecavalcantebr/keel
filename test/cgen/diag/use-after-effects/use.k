module use;
import ring;

// the argument of the consuming call is the consumption, not a use after it
pub void consume_once(consumes ring.Ring *r) {
    ring.drop(r);
}

// the use after the consumption is reported, once, at the use
pub void consume_then_use(consumes ring.Ring *r) {
    ring.drop(r);
    ring.touch(r);                                /* DIAG: child-region-after-invalidation */
}

// the argument of a call that invalidates children is a use before the effect
pub void clear_then_read(invalidates ring.Ring *r) {
    i32 *p = ring.raw(r);
    ring.clear(r);
    i32 v = *p;                                   /* DIAG: child-region-after-invalidation */
    (void)v;
}

// `*p = 1;` writes through `p`: it is a use of `p`, and does not reassign it (§4.12, rule 10)
pub void store_after(invalidates ring.Ring *r) {
    i32 *p = ring.raw(r);
    ring.clear(r);
    *p = 1;                                       /* DIAG: child-region-after-invalidation */
}

pub void store_valid(ring.Ring *r) {
    i32 *p = ring.raw(r);
    *p = 1;
}

// `p = ...;` does reassign `p`: the new value is valid
pub void reassign(invalidates ring.Ring *r) {
    i32 *p = ring.raw(r);
    ring.clear(r);
    p = ring.raw(r);
    *p = 3;
}
