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
