module mx dim ROWS, COLS;

/* `box` tem só binders de dimensão (sem `type`): a única instância de
   `zeros` vem do alvo da atribuição (spec §4.4, item 4), não de um
   receptor (`box` não aparece como argumento). */
pub modifier box { double data[ROWS][COLS]; }

pub inline box zeros(void) {
    box v = {0};
    return v;
}

pub inline void set(box *b, size_t i, size_t j, double x) { b->data[i][j] = x; }
pub inline double get(box *b, size_t i, size_t j) { return b->data[i][j]; }
