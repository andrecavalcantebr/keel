module app.ap;
import keel.buffer as buffer types;
import keel.range  as range  types;

priv i32 acc = 0;
priv void dobra (i32 v, size_t i)        { acc += v * 2 + (i32)i; }
priv void escala(i32 v, size_t i, i32 k) { acc += v * k + (i32)i; }
pub  i32  soma_acc(void) { return acc; }

/* `apply` é `foreach` de corpo fixo. `fn` recebe elemento e índice, nessa
   ordem, e os argumentos de contexto atravessam opacos depois deles (§4.7). */
pub void percorre(buffer i32 *xs) {
    apply(i32, xs, dobra);
    apply(i32, xs, escala, 3);
}

/* `range` é contável: declara `first`, `limit`, `length` e `get`, então serve
   à forma de um binder e à de dois. */
pub size_t soma_range(range r) {
    size_t t = 0;
    foreach (auto v : r) t += v;
    foreach (size_t v, size_t i : r) t += v * i;
    return t;
}
