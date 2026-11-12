module lib;
// an error of analysis in the body of an imported module says nothing about its interface

pub typedef struct { i32 *at; } Box;
pub inline child i32 *raw(parent Box *b)  { return b->at; }
pub inline void clear(invalidates Box *b) { (void)b; }

pub inline i32 *peek(Box *b) {
    i32 *q = raw(b);
    return q;                                     /* DIAG: role-missing */ /* DIAG: role-missing */
}
