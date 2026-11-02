module app.xc;

extern_c [type_h] {
    typedef struct { int a; } Raw;
}

pub int before(Raw *r) { return r->a; }

extern_c {
    static inline int twice(int v) { return v * 2; }
}

pub int after(Raw *r) { return twice(r->a); }

priv extern_c {
    static int cache = 7;
}

pub int hidden(void) { return cache; }
