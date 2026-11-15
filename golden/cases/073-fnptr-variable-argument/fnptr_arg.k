module fnptr_arg;

priv u64 hash(const i32 *k) { return (u64)*k; }
priv u64 apply(u64 (*f)(const i32 *), const i32 *x) { return f(x); }
priv u64 forward(u64 (*h)(const i32 *), const i32 *x) { return apply(h, x); }

pub u64 run(void) { i32 v = 3; return forward(hash, &v); }
