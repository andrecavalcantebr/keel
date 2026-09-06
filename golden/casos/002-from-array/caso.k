module app.pool;
import keel.arena as arena types;

alignas(64) array u8 memo[65536];

pub bool inicia(arena *a) {
    return arena.from_array(a, memo);
}
