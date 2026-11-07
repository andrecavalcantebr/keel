module tf;
import keel.arena as arena types;
import pt as pt types;

pub typedef struct { i32 x, y; } local_point;

pub void ok(arena *a, size_t n) {
    i32 *p1 = arena.alloc(a, i32, n);
    local_point *p2 = arena.alloc(a, local_point, n);
    (void)p1; (void)p2;
}

pub void bad(arena *a, size_t n) {
    pt.point *p = arena.alloc(a, pt.point, n);   /* DIAG: type-arg-foreign */
    (void)p;
}
