module adim;
import keel.arena as arena types;

/* A dimension that is a constant expression (`1<<10`), not a plain number: it reaches
   `from_array` as its length argument, copied as written (keel_coll finding A5). */
priv array u8 memo[1<<10];

pub bool start(arena *a) {
    return arena.from_array(a, memo);
}
