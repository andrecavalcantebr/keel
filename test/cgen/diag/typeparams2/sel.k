module sel type T, U;
pub modifier pair { T a; U b; };
pub inline pair make(type T, T a) { pair p = {0}; p.a = a; return p; }   /* DIAG: partial-instance-selection */
