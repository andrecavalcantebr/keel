module app.bs;
import keel.buffer   as buffer   types;
import keel.slice    as slice    types;
import keel.parallel as parallel;

/* Busca: `win` é o único veredito que conta para a política, então `ok` só é
   verdadeiro se alguém achou (§4.8). Quem observa o pedido de interrupção
   apenas para de procurar — a consulta não é saída. */
pub bool achou_alguem(buffer i32 *xs, i32 alvo, buffer size_t *onde) {
    parallel busca ANY (size_t w : 0..4; slice i32 parte : xs; (alvo, onde)) {
        foreach (i32 x, size_t i : parte) {
            if (parallel.interrupted(busca)) break;
            if (x == alvo) { buffer.set(onde, w, i + 1); win; }
        }
    }
    return parallel.ok(busca);
}
