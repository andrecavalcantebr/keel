module app3.a02;
pub typedef struct { int x, y; } Pt;
pub typedef struct { array Pt pts[4]; size_t n; } Poly;
pub int sum_x(Poly *p) {
    int t = 0;
    for (size_t i = 0; i < p->n; i++) t += p->pts[i].x;
    return t;
}
