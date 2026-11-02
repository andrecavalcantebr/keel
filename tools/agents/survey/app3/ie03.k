module app3.ie03;
extern_c [type_h] {
    typedef struct { int a; } Raw;
}
pub int f(Raw *r) { return r->a; }
