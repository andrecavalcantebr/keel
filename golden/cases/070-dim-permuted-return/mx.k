module mx dim ROWS, COLS;

/* `transpose` returns the module's own modifier with the dimensions swapped:
   `box(COLS, ROWS)` is another instance of the module, with the numbers of the
   instance that is being emitted (spec §4.4, item 4). */
pub modifier box { double data[ROWS][COLS]; }

pub inline box(COLS, ROWS) transpose(box *b) {
    box(COLS, ROWS) t = {0};
    for (size_t i = 0; i < ROWS; ++i)
        for (size_t j = 0; j < COLS; ++j)
            t.data[j][i] = b->data[i][j];
    return t;
}

pub inline void set(box *b, size_t i, size_t j, double x) { b->data[i][j] = x; }
pub inline double get(box *b, size_t i, size_t j) { return b->data[i][j]; }
