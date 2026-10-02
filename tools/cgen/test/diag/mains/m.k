module m;
priv extern_c [type_h] { struct p { int x; }; }   /* DIAG: type-layer-on-priv-extern-c */
extern_c { int main(void) { return 0; } }          /* DIAG: main-in-extern-c */
priv int main(int argc, char **argv) {             /* DIAG: private-main */
    extern_c { int y; }                            /* DIAG: nested-extern-c */
    return argc;
}
