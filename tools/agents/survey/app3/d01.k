module app3.d01;
pub int x = 1, *px = &x;
pub int get(void) { return *px; }
