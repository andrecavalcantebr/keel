module app3.ie01;
import_c <stdio.h>;
priv extern_c { static int cache = 1; }
pub int f(void) { puts("x"); return cache; }
