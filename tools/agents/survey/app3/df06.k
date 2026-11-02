module app3.df06;
pub typedef struct { int v[3]; } Box;
pub int f(Box *p, int i) {
    defer p->v[i] = 0;
    p->v[i] = 7;
    return p->v[i];
}
