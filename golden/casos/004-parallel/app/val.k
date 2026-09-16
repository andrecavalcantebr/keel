module app.val;
import keel.buffer   as buffer   types;
import keel.slice    as slice    types;
import keel.parallel as parallel;

/* Política ALL: não há bandeira, todo worker roda inteiro, e `failed` é
   determinístico (§4.8). É o que torna este caso executável.
   `parallel` não percorre: entrega a parte, e percorrer é do corpo. */
pub bool tem_negativo(buffer i32 *xs) {
    parallel valida ALL (size_t w : 0..4; slice i32 parte : xs) {
        foreach (i32 x, size_t i : parte) {
            if (x < 0) fail;
        }
    }
    return parallel.failed(valida);
}
