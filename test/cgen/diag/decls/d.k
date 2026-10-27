module d;
import lib.blk as blk types;

constexpr size_t M = 2;
constexpr size_t Z = 0;
pub tags Color [RED, GREEN];
pub tags [A, B];                             /* DIAG: unnamed-tags */
pub tags Empty [];                           /* DIAG: empty-tags */
pub tags Mixed [P = 1, Q];                   /* DIAG: partial-tag-values */
pub tags Odd [X = y];                        /* DIAG: nonconstant-tag-value */
pub tags Color [BLUE];                       /* DIAG: duplicate-tags-name */
pub static int counter;                      /* DIAG: pub-static */
static inline int helper(void) { return 0; } /* DIAG: inline-without-visibility */
static typedef int handle;                   /* DIAG: static-on-type */
pub int keel_count;                          /* DIAG: reserved-name */
pub int foreach;                             /* DIAG: keel-name-shadowed */
blk.blk(M) Color i32 ok;
blk.blk(n) Color i32 v1;                     /* DIAG: nonconstant-dim */
blk.blk(Z) Color i32 v2;                     /* DIAG: dim-below-one */
blk.blk(2) i32 i32 v3;                       /* DIAG: undeclared-tags */
pub typedef int Color_RED;                   /* DIAG: canonical-name-collision */
constexpr int TABLE[2] = {1, 2};             /* DIAG: nonscalar-constexpr */
constexpr int (*fp) = 0;                     /* DIAG: constexpr-name-missing */
pub void f(void) {
    constexpr size_t K = {3};                /* DIAG: nonscalar-constexpr */
}
pub int total;
pub void g(void) {
    handle total;                            /* DIAG: symbol-redeclaration */
    blk.blk(2) Color i32 x = {0};
    blk.blk(2) Color i32 y = {0};
    x = y;
}
pub int array;                               /* DIAG: keel-name-shadowed */
pub int win(void) { return 0; }
pub void h(void) {
    int parallel = 0;                        /* DIAG: keel-name-shadowed */
    (void)parallel;
}
