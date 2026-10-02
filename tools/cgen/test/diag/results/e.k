module e;
import keel.outcome as outcome types;
import keel.corot as corot types;

pub typedef struct { i32 x; outcome i32 r; } Box;
pub outcome i32 get(void);
pub corot step(void);

pub i32 use(Box *b, i16 *p) {
    outcome i32 a = get() else return 1;
    outcome i32 c = get() else 0;
    outcome i32 d else 0;                                   /* DIAG: else-without-initializer */
    outcome i32 e = get(), f = get() else 0;                /* DIAG: else-multiple-declarators */
    b->r = get() else 0;                                    /* DIAG: else-on-complex-target */
    *p = 1 else 0;                                          /* DIAG: else-on-complex-target */
    i16 res = c else 0;                                     /* DIAG: protocol-not-satisfied */
    corot s = step() else return 2;                         /* DIAG: protocol-not-satisfied */
    corot t = step() else 0;                                /* DIAG: protocol-not-satisfied */
    a = get() else break;
    if (a.code) return 3; else return 4;
}
