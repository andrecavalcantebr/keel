module gen type T;
pub modifier box { T v; };
pub inline T get(box *b) { return b->v; }
pub size_t put(box *b, T v);
pub size_t counter(void);                   /* DIAG: nonparametric-out-of-line */
pub int shared_flag;                        /* DIAG: nonparametric-out-of-line */

pub modifier instance { T v; };            /* DIAG: modifier-named-instance */ /* DIAG: keel-name-shadowed */
pub inline size_t twice(box *b, size_t T) { return T; }   /* DIAG: parameter-name-reuse */
