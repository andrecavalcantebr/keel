module p;

pub protocol A type C { size_t length(C *x); };
pub protocol B type C { i32 length(C *x); };
pub protocol AB [A, B];                      /* DIAG: protocol-verb-conflict */
pub protocol Ghost [A, Nowhere];             /* DIAG: unknown-protocol */
pub protocol X [Y];                          /* DIAG: circular-protocol */
pub protocol Y [X];                          /* DIAG: circular-protocol */
pub protocol Loose type C {
    type idx;                                /* DIAG: associated-type-unbound */
    size_t count(C *x);
};

pub protocol First type C { type index; index first(C *x); };
pub protocol Last type C { type index; index last(C *x); };
pub protocol Span [First, Last];
pub typedef struct { int a; } Odd;
pub inline int first(Odd *o) { return o->a; }
pub inline long last(Odd *o) { return o->a; }
pub size_t span(Span C x) { return 0; }
pub size_t use(Odd *o) { return span(o); }      /* DIAG: associated-type-conflict */
