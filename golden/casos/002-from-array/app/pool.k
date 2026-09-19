module app.pool;
import keel.arena as arena types;

/* `alignas` atravessa verbatim (§3.1). Ele é ECONOMIA, não correção: sem ele a
   arena funciona igual e gasta padding na primeira alocação (§4.4). */
priv alignas(64) array u8 memo[65536];

pub bool inicia(arena *a) {
    return arena.from_array(a, memo);
}
