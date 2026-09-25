module app.ofr;

import keel.slice   as slice   types;
import keel.buffer  as buffer  types;
import keel.range   as range   types;
import keel.outcome as outcome types;
import keel.array   as array;
pub array i32 v[6] = {10, 11, 12, 13, 14, 15};

/* slice.of over an array: the core supplies the extent from the table
   (backend §5.2); the three-argument and range forms go through the slice's
   own `of`, so every argument is evaluated once. */
pub slice i32 whole(void) { return slice.of(v); }
pub slice i32 middle(size_t a, size_t b) { return slice.of(v, a, b); }
pub slice i32 by_range(range r) { return slice.of(v, r); }

/* over a slice and over a buffer: the verb is chosen by arity (§2.1). */
pub slice i32 same(slice i32 s) { return slice.of(s); }
pub slice i32 sub(slice i32 s, range r) { return slice.of(s, r); }
pub slice i32 from_buffer(range r) {
    buffer i32 b = buffer.of(v);
    return slice.of(b, r);
}

/* the constant is core (spec §4.2); element access is keel.array (§5.3) */
pub size_t count(void) { return keel.length(v); }
pub i32 second(void) { return array.get(v, 1); }
pub void put(size_t i, i32 x) { array.set(v, i, x); }
pub i32 *at_ptr(size_t i) { return array.ptr(v, i); }
pub i32 checked(size_t i) {
    outcome i32 r = array.at(v, i) else return -1;
    return outcome.value(r);
}

/* range: `a..b` is range.of(a, b), and an inverted interval is empty. */
pub size_t inverted(void) { range r = range.of(5, 2); return range.length(r); }
pub size_t literal(void) { range r = 2..5; return range.length(r); }
