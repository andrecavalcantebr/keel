module app.packed;
import keel.bitbuffer as bitbuffer types;
import keel.bitslice  as bitslice  types;
import keel.arena     as arena     types;
import keel.outcome   as outcome   types;
import keel.range     as range     types;
import keel.slice     as slice;

/* The eight values 0..7 at three bits each take three bytes, least
   significant bit first (spec §5.8, layout): the proof reads them raw. */
pub void layout(u8 *store) {
    bitbuffer(3) u8 g = bitbuffer.from(store, 8);
    for (u8 i = 0; i < 8; ++i) bitbuffer.push(g, i);
}

/* One round trip per width and element type: fill to capacity, a push that
   does not fit, then read everything back. Zero is success. */
pub i32 widths(arena *a) {
    bitbuffer(3) u8 g3 = bitbuffer.make(a, 100);
    for (u8 i = 0; i < 100; ++i) if (!bitbuffer.push(g3, i % 8)) return 1;
    if (bitbuffer.push(g3, 1)) return 2;
    for (size_t i = 0; i < 100; ++i) if (bitbuffer.get(g3, i) != i % 8) return 3;
    if (bitbuffer.bytes(g3) != 38) return 4;

    bitbuffer(5) u8 g5 = bitbuffer.make(a, 33);
    for (u8 i = 0; i < 33; ++i) bitbuffer.push(g5, (u8)(i * 7) % 32);
    for (u8 i = 0; i < 33; ++i) if (bitbuffer.get(g5, i) != (u8)(i * 7) % 32) return 5;

    bitbuffer(13) u16 g13 = bitbuffer.make(a, 9);
    for (u16 i = 0; i < 9; ++i) bitbuffer.push(g13, 8191 - i * 997);
    for (u16 i = 0; i < 9; ++i) if (bitbuffer.get(g13, i) != 8191 - i * 997) return 6;

    bitbuffer(33) u64 g33 = bitbuffer.make(a, 10);
    for (u64 i = 0; i < 10; ++i) bitbuffer.push(g33, (i << 29) | i);
    for (u64 i = 0; i < 10; ++i) if (bitbuffer.get(g33, i) != ((i << 29) | i)) return 7;

    bitbuffer(1) bool flags = bitbuffer.make(a, 20);
    for (int i = 0; i < 20; ++i) bitbuffer.push(flags, i % 3 == 0);
    for (int i = 0; i < 20; ++i) if (bitbuffer.get(flags, i) != (i % 3 == 0)) return 8;
    return 0;
}

/* `x[a..b]`, a view of a view, and a write through a view that reaches the
   buffer. The view starts mid-byte, so `bit` is not zero. */
pub i32 views(arena *a) {
    bitbuffer(3) u8 g = bitbuffer.make(a, 64);
    for (u8 i = 0; i < 64; ++i) bitbuffer.push(g, i % 8);
    bitslice(3) u8 v = g[5..60];
    if (bitslice.length(v) != 55) return 1;
    for (size_t i = 0; i < bitslice.length(v); ++i) if (bitslice.get(v, i) != (i + 5) % 8) return 2;
    bitslice(3) u8 w = v[3..20];
    for (size_t i = 0; i < bitslice.length(w); ++i) if (bitslice.get(w, i) != (i + 8) % 8) return 3;
    bitslice.set(w, 0, 7);
    if (bitbuffer.get(g, 8) != 7 || bitbuffer.get(g, 7) != 7 || bitbuffer.get(g, 9) != 1) return 4;
    bitslice(3) u8 r = bitbuffer.as_slice(g, range.of(10, 14));
    if (bitslice.length(r) != 4 || bitslice.get(r, 3) != 5) return 5;
    bitslice(3) u8 all = slice.of(g);
    if (bitslice.length(all) != 64) return 6;
    return 0;
}

/* `foreach` by value: an element has no address, so `IndexGet` is what serves. */
pub u32 total(arena *a) {
    bitbuffer(4) u8 g = bitbuffer.make(a, 30);
    for (u8 i = 0; i < 30; ++i) bitbuffer.push(g, i % 16);
    u32 sum = 0;
    foreach (u8 q, size_t i : g) sum += (u32)q * (u32)(i + 1);
    bitslice(4) u8 v = g[4..10];
    foreach (u8 q, size_t i : v) sum += q;
    return sum;
}

/* `pop`, `at`, `clear`, and the answer of an empty or out-of-range access. */
pub i32 stack(arena *a) {
    bitbuffer(5) u8 g = bitbuffer.make(a, 3);
    bitbuffer.push(g, 20); bitbuffer.push(g, 21); bitbuffer.push(g, 22);
    outcome u8 top = bitbuffer.pop(g);
    if (outcome.failed(top) || outcome.value(top) != 22 || bitbuffer.length(g) != 2) return 1;
    outcome u8 hit = bitbuffer.at(g, 1);
    if (outcome.failed(hit) || outcome.value(hit) != 21) return 2;
    if (!outcome.failed(bitbuffer.at(g, 2))) return 3;
    bitbuffer.clear(g);
    if (bitbuffer.length(g) != 0 || bitbuffer.capacity(g) != 3) return 4;
    if (!outcome.failed(bitbuffer.pop(g))) return 5;
    bitbuffer(5) u8 none = bitbuffer.from(NULL, 10);
    if (bitbuffer.capacity(none) != 0 || bitbuffer.push(none, 1)) return 6;
    return 0;
}

/* `clone` of a view that starts mid-byte realigns it to bit 0 of its own bytes. */
pub i32 cloned(arena *a) {
    bitbuffer(5) u8 g = bitbuffer.make(a, 30);
    for (u8 i = 0; i < 30; ++i) bitbuffer.push(g, i);
    outcome bitbuffer(5) u8 c = bitbuffer.clone(a, g) else { return 1; }
    bitbuffer(5) u8 d = outcome.value(c);
    for (u8 i = 0; i < 30; ++i) if (bitbuffer.get(d, i) != i) return 2;
    bitslice(5) u8 v = g[3..27];
    outcome bitslice(5) u8 vc = bitslice.clone(a, v) else { return 3; }
    bitslice(5) u8 e = outcome.value(vc);
    if (bitslice.length(e) != 24) return 4;
    for (size_t i = 0; i < 24; ++i) if (bitslice.get(e, i) != i + 3) return 5;
    return 0;
}
