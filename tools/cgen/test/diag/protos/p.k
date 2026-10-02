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
