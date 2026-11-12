module use;
import ring;

// `ptr` inherits `parent` and `child`: the pointer it returns comes from the ring
pub void inherited(invalidates ring.Ring *r) {
    i32 *p = ring.ptr(r, 0);
    ring.clear(r);
    i32 v = *p;                                   /* DIAG: child-region-after-invalidation */
    (void)v;
}

// `length` returns a size, not a child: nothing to invalidate
pub void not_a_child(invalidates ring.Ring *r) {
    size_t n = ring.length(r);
    ring.clear(r);
    size_t m = n;
    (void)m;
}

// `peek` writes none and no protocol names it: its result is not a child
pub void no_protocol(invalidates ring.Ring *r) {
    i32 *p = ring.peek(r);
    ring.clear(r);
    i32 v = *p;
    (void)v;
}
