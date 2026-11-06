module win;

/* `window` is countable by its own bounds: it declares neither `get` nor
   `ptr`, so it meets only `Countable` (spec §5.1), the one-binder `foreach`
   (spec §4.7, backend §5.7 rule 6). */
pub typedef struct { size_t lo, hi; } window;

pub inline size_t first(window *w) { return w->lo; }
pub inline size_t limit(window *w) { return w->hi; }

pub size_t sum_param(window *w) {
    size_t total = 0;
    foreach (auto i : w) {
        total += i;
    }
    return total;
}

pub size_t sum_local(void) {
    window w = { 2, 5 };
    size_t total = 0;
    foreach (auto i : w) {
        total += i;
    }
    return total;
}
