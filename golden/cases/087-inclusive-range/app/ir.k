module app.ir;
import keel.slice as slice types;
import keel.array as array;
import keel.range as range types;
import_c <stdio.h>;
import_c <stdlib.h>;

/* a..b is a..<b (half-open); a..=b includes b (spec §4.5). Open ends take the
   container to its last element, whichever marker is written. `keel_inclusive`
   is what b + 1 goes through: debug aborts at SIZE_MAX, release saturates there. */
pub int main(int argc, char **argv) {
    array i32 v[6] = {10, 20, 30, 40, 50, 60};
    slice i32 s = slice.of(v, 0, 6);
    size_t sum = 0;
    foreach (size_t i : 1..=3) sum += i;
    foreach (size_t i : 1..<3) sum += i * 10;
    foreach (size_t i : 1..3) sum += i * 100;
    if (sum != 6 + 30 + 300) return 1;
    range r = 2..=4;
    if (r.first != 2 || r.limit != 5) return 2;
    slice i32 a = s[1..=3];
    slice i32 b = s[..=1];
    slice i32 c = s[4..=];
    slice i32 d = s[4..<];
    slice i32 e = s[4..];
    slice i32 f = s[..<2];
    slice i32 g = s[2..<5];
    slice i32 h = v[1..=2];
    if (slice.length(a) != 3 || slice.length(b) != 2) return 3;
    if (slice.length(c) != 2 || slice.length(d) != 2 || slice.length(e) != 2) return 4;
    if (slice.length(f) != 2 || slice.length(g) != 3 || slice.length(h) != 2) return 5;
    size_t top = argc > 1 ? (size_t)-1 : 3;
    slice i32 t = s[0..=top];
    if (slice.length(t) != 4) return 6;
    puts("ok");
    return 0;
}
