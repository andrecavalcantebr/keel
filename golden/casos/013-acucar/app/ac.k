module app.ac;
import keel.buffer as buffer types;
import keel.slice  as slice  types;
import_c <stdio.h>;

/* `x[i]` é `*ptr(x,i)` — lvalue, e o lowering é por função, nunca por macro:
   é o que garante que `x[i++]` incremente `i` uma vez só (§5.3). */
pub i32 troca(buffer i32 *b, size_t i) {
    b[i] = b[i] + 1;
    return b[i];
}

pub i32 uma_vez(buffer i32 *b, size_t *i) {
    b[(*i)++] = 9;              /* o índice é avaliado exatamente uma vez */
    return (i32)*i;
}

/* `array` multidimensional: a forma com vírgula vira colchetes, sem função. */
pub i32 cubo(void) {
    array i32 v[2,3,4];
    v[1,2,3] = 7;
    return v[1][2][3];
}

/* Recorte: rvalue, semiaberto. As quatro formas do §4.6. */
pub size_t recorta(buffer i32 *b) {
    slice i32 meio = b[2..7];
    slice i32 fim  = b[5..];
    slice i32 ini  = b[..3];
    slice i32 todo = b[..];
    return slice.length(meio) + slice.length(fim)
         + slice.length(ini)  + slice.length(todo);
}
