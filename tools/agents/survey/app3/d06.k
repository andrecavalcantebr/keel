module app3.d06;
pub typedef struct { struct In { int v; } in; int w; } Out;
pub int f(Out *o) { return o->in.v + o->w; }
