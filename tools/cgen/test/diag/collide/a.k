module a;
import a.b;
import keel.slice as slice;                   /* DIAG: alias-type-collision */
pub int b_c;                                 /* DIAG: symbol-collision */
pub typedef struct { int x; } slice;
