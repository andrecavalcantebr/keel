module t;
import_c <string.h>;

pub protocol Sized type C { size_t length(C *x); };

pub void *grab(type X, size_t n) { return (X *)0 + n; }              /* DIAG: type-param-outside-size */
pub size_t ok(type Y) { return sizeof(Y) + alignof(Y); }
pub size_t bad(type i32) { return sizeof(i32); }                      /* DIAG: type-param-shadows-type */
pub size_t sz(Sized C x) { return length(&x) + capacity(&x); }       /* DIAG: verb-not-in-protocol */
