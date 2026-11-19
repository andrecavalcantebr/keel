module app.gfp dim W type T;

/* A `static_assert` that names `W` and `T` is a check of each instance's own
   values: it is written once and every instance carries it, with the numbers
   and the type substituted (spec §4.3, rule 20, and §5.8, rule 1). */
static_assert(W >= 1 && W <= 8 * sizeof(T), "gfp: W bits do not fit in a T");

/* `twice` names neither a binder nor the modifier: it belongs to the module, is
   emitted once, and is called as the verb of a plain module, from outside and
   from the body of an instance's verb. */
pub inline size_t twice(size_t x) { return 2 * x; }

pub modifier cell { T v; }

pub inline size_t width2(cell *c) { (void)c; return twice(W); }
