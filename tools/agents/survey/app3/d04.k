module app3.d04;
pub typedef struct { int x, y; } Pt;
pub Pt shift(Pt p, int dx) { p.x += dx; return p; }
