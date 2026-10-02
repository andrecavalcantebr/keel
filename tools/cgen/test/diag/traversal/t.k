module t;
import keel.buffer as buffer types;
import keel.range as range types;
import keel.tagged as tagged types;

pub typedef struct { i32 n; } Bare;
pub typedef struct { i32 code; } Odd;
pub tags Light [RED, AMBER, GREEN];
pub inline i32 tag(Odd *o) { return o->code; }

pub void use(buffer i32 *xs, Bare *b, array i32 v[size_t n], tagged Light void *s, Odd *o, i32 k) {
    if (k) defer use(xs, b, v, s, o, 0);               /* DIAG: defer-without-braces */
    if (k) { defer use(xs, b, v, s, o, 0); }           /* DIAG: defer-in-control-block */
    defer [later k] use(xs, b, v, s, o, 0);            /* DIAG: later-with-capture */
    walk (i32 *p : xs) { }                             /* DIAG: walk-without-cursor */
    walk (i32 *p, buffer.cursor c : xs) { }
    walk (i32 *p, buffer.cursor c : v) { }             /* DIAG: protocol-not-satisfied */
    foreach (size_t i, size_t j : 0..4) { }            /* DIAG: foreach-two-binders-on-literal */
    foreach (size_t *i : 0..4) { }                     /* DIAG: pointer-binder-on-range */
    foreach (i32 x, int i : xs) { }                    /* DIAG: index-not-size-t */
    foreach (i32 x, size_t i : b) { }                  /* DIAG: protocol-not-satisfied */
    foreach (i32 x, size_t i : v) { }
    foreach (i32 x, size_t i : xs) { buffer.clear(xs); }   /* DIAG: mutation-during-traversal */
    foreach (i32 x, size_t i : xs) buffer.pop(xs);         /* DIAG: mutation-during-traversal */
    match (s) {                                        /* DIAG: tag-without-label */
        RED: break;
        AMBER: break;
        BLUE: break;                                   /* DIAG: tag-not-in-set */
        RED: break;                                    /* DIAG: duplicate-tag */
    }
    match (s) {                                        /* DIAG: tag-without-label */
        RED: break;
        AMBER: break;
    }
    match (b) { RED: break; }                          /* DIAG: protocol-not-satisfied */
    match (o) { RED: break; }                          /* DIAG: match-without-tags */
}
pub tags Other [RED2, BLUE2];
pub void paint(tagged Light void *s) {
    tagged.mark(s, BLUE2);                             /* DIAG: tag-from-other-set */
    tagged.mark(s, AMBER);
    match (s) {                                        /* DIAG: tag-without-label */ /* DIAG: tag-without-label */
        RED: break;
        RED2: break;                                   /* DIAG: tag-from-other-set */
    }
}
