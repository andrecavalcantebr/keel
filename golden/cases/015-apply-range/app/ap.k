module app.ap;
import keel.buffer as buffer types;
import keel.range  as range  types;

priv i32 acc = 0;
priv void doubler (i32 v, size_t i)        { acc += v * 2 + (i32)i; }
priv void scale(i32 v, size_t i, i32 k) { acc += v * k + (i32)i; }
pub  i32  sum_acc(void) { return acc; }

/* `apply` is a `foreach` with a fixed body. `fn` takes element and index, in
   that order, and the context arguments cross over opaquely after them
   (§4.7). */
pub void traverse(buffer i32 *xs) {
    apply(i32, xs, doubler);
    apply(i32, xs, scale, 3);
}

/* `range` is countable: it declares `first`, `limit`, `length` and `get`, so
   it serves both the one-binder and the two-binder forms. */
pub size_t sum_range(range r) {
    size_t t = 0;
    foreach (auto v : r) t += v;
    foreach (size_t v, size_t i : r) t += v * i;
    return t;
}
