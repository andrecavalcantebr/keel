module app.val;
import keel.buffer as buffer types;

/* Política ALL: não há bandeira, toda faixa roda inteira, e `failed` é
   determinístico (§4.7). É o que torna este caso executável. */
pub bool tem_negativo(buffer i32 *xs) {
    parallel valida ALL (size_t w : 0..4; i32 x, size_t i : xs; ()) {
        if (x < 0) fail;
    }
    return failed(valida);
}
