module app.nd;

pub tags Kind [LIT, ADD];

/* the column holds a set: its element is the operand of `match` (spec §4.11) */
pub extent struct nodes [n, cap] {
    array Kind *kind;
    size_t n, cap;
};

pub i32 weight(struct nodes *t, size_t i) {
    i32 r = 0;
    match (t->kind[i]) {
        LIT:
            r = 1;
        ADD:
            r = 2;
    }
    return r;
}
