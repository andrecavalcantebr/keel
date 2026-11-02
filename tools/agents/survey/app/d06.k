module app.d06;
priv struct Local { int a; };
pub int f(void) { struct Local l; l.a = 1; return l.a; }
