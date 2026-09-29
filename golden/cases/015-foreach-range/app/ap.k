module app.ap;
import keel.buffer as buffer types;
import keel.range  as range  types;

priv i32 acc = 0;
priv void doubler (i32 v, size_t i)        { acc += v * 2 + (i32)i; }
priv void scale(i32 v, size_t i, i32 k) { acc += v * k + (i32)i; }
pub  i32  sum_acc(void) { return acc; }

/* Two-binder `foreach` over a buffer: the container and its length are taken
   once, on entry, and the body calls a function with the element, the index
   and whatever else it needs (§4.7). */
pub void traverse(buffer i32 *xs) {
    foreach (i32 v, size_t i : xs) doubler(v, i);
    foreach (i32 v, size_t i : xs) scale(v, i, 3);
}

/* `range` is countable: it declares `first`, `limit`, `length` and `get`, so
   it serves both the one-binder and the two-binder forms. */
pub size_t sum_range(range r) {
    size_t t = 0;
    foreach (auto v : r) t += v;
    foreach (size_t v, size_t i : r) t += v * i;
    return t;
}
