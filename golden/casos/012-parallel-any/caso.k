module app.bs;
import keel.buffer as buffer types;
import_c <stdio.h>;

/* ANY: contador e bandeira atômicos, locais do gestor. `interrupted` é o ponto
   de interrupção declarado, e é obrigatório fora de ALL (error 88). */
pub bool achou_alguem(buffer i32 *xs, i32 alvo, buffer size_t *onde) {
    parallel busca ANY (size_t w : 0..4; i32 x, size_t i : xs; (alvo)) {
        interrupted;
        if (x == alvo) { buffer.set(onde, w, i); win; }
    }
    return failed(busca) == false && ok(busca) == false;
}
