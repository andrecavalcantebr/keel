module s;

pub keel_code void twice(keel_code C) { C; C; }  /* DIAG: not-in-v0 */ /* DIAG: not-in-v0 */
pub keel_code i32 clamp(dim N, i32 v) { return v; }  /* DIAG: not-in-v0 */ /* DIAG: not-in-v0 */
pub invalidates i32 bad(i32 x) { return x; }     /* DIAG: role-position */
pub i32 worse(parent type T, i32 x) { return x; } /* DIAG: role-position */
pub child i32 *fine(parent i32 *p) { return p; }
