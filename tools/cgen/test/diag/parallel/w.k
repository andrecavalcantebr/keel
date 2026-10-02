module w;
import keel.buffer as buffer types;
import keel.slice as slice types;
import keel.parallel as parallel;

constexpr size_t K = 4;
pub typedef struct { i32 n; } Bare;

pub bool run(buffer i32 *xs, Bare *b, size_t k, i32 seen, i32 *out) {
    parallel ALL (size_t w : 0..4; slice i32 part : xs) { }                 /* DIAG: unnamed-parallel */
    parallel a ALL (size_t w : 0..K; slice i32 part : xs) { win; }
    parallel c ANY (size_t w : 0..k; slice i32 part : xs) { }               /* DIAG: nonconstant-parallel */
    parallel d seen (size_t w : 0..4; slice i32 part : xs) { }              /* DIAG: nonconstant-parallel */
    parallel a ALL (size_t w : 0..4; slice i32 part : xs) { }               /* DIAG: duplicate-parallel-name */
    parallel e ALL (size_t w : 0..4; buffer i32 part : xs) { }              /* DIAG: partition-type-mismatch */
    parallel f ALL (size_t w : 0..4; slice i32 part : b) { }                /* DIAG: protocol-not-satisfied */
    parallel g 2 (size_t w : 0..4; slice i32 part : xs; (seen, out)) {
        foreach (i32 x, size_t i : part) {
            if (x == 0) fail;
            if (x == 1) win;
        }
        seen = 1;                                                           /* DIAG: captured-write */
        seen++;                                                             /* DIAG: captured-write */
        *out = 1;
        buffer.push(xs, 1);                                                 /* DIAG: mutation-during-traversal */
        if (seen == 2) return false;                                        /* DIAG: return-in-parallel */
        parallel h ALL (size_t v : 0..2; slice i32 q : part) { }            /* DIAG: nested-parallel */
    }
    win;                                                                    /* DIAG: flow-verb-outside-parallel */
    return parallel.ok(a);
}
