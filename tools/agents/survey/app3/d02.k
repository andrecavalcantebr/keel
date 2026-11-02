module app3.d02;
pub typedef struct { int *a, **b, c; } Ptrs;
pub int f(Ptrs *p) { return p->c + *p->a; }
