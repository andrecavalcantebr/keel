module app.pool;
import keel.arena as arena types;

/* `alignas` crosses over verbatim (§3.1). It buys ECONOMY, not correctness:
   without it the arena still works and spends padding on the first
   allocation (§4.4). */
priv alignas(64) array u8 memo[65536];

pub bool start(arena *a) {
    return arena.from_array(a, memo);
}
