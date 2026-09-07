module app.val;
import keel.buffer as buffer types;
import_c <stdio.h>;

/* Política ALL: não há bandeira, toda faixa roda inteira, e `failed` é
   determinístico (§4.7). É o que torna este caso executável. */
pub bool tem_negativo(buffer i32 *xs) {
    parallel valida ALL (size_t w : 0..4; i32 x, size_t i : xs; ()) {
        if (x < 0) fail;
    }
    return failed(valida);
}

pub int main(int argc, char **argv) {
    array i32 v[16];
    buffer i32 b = buffer.of(v);
    foreach (i32 e, size_t i : b) buffer.set(b, i, 1);
    if (tem_negativo(b)) return 1;              /* nenhum negativo */
    buffer.set(b, 13, -7);                      /* faixa 3 */
    if (!tem_negativo(b)) return 2;             /* o `fail` tem de sobreviver */
    puts("ok");
    return 0;
}
