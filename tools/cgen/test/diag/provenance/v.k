module v;
import keel.arena as arena types;
import keel.buffer as buffer types;
import keel.slice as slice;
import_c <stdio.h>;

priv void use(i32 *p) { (void)p; }

pub void spec_example(void) {
    array u8 storage[1024];
    arena a;
    arena.from_array(a, storage);
    i32 *p = arena.alloc(a, i32, 10);
    arena.reset(a);
    use(p);                                       /* DIAG: child-region-after-invalidation */
    p = arena.alloc(a, i32, 10);
    use(p);
}

pub i32 *escape(void) {
    array u8 storage[64];
    arena a;
    arena.from_array(a, storage);
    i32 *p = arena.alloc(a, i32, 4);
    return p;                                     /* DIAG: region-escape */
}

pub u8 *escape_array(void) {
    array u8 local[8];
    return local;                                 /* DIAG: region-escape */
}

static array u8 kept[64];
pub i32 *kept_ok(arena *a) {
    arena.from_array(a, kept);
    i32 *p = arena.alloc(a, i32, 4);
    return p;
}

pub void views(buffer i32 *b) {
    slice.slice i32 s = b[0..2];
    buffer.clear(b);
    foreach (i32 *e, size_t i : b) { *e = 0; }
    (void)s;
}
