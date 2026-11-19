module app.chk;
import keel.bitbuffer as bitbuffer types;
import keel.bitslice  as bitslice  types;
import_c <stdlib.h>;

/* One run takes one check by its number (golden DEBUG): each branch breaks a
   rule that only the debug build verifies (backend §5.17), and aborts there. */
pub int main(int argc, char **argv) {
    array u8 store[4] = {0};
    bitbuffer(3) u8 g = bitbuffer.of(&store[0], 8);
    bitslice(3) u8 v = g[2..6];
    size_t i = argc > 2 ? (size_t)atoi(argv[2]) : 8;
    int which = argc > 1 ? atoi(argv[1]) : 0;
    if (which == 1) return bitbuffer.get(g, i);
    if (which == 2) { bitbuffer.set(g, i, 0); return 0; }
    if (which == 3) { bitbuffer.set(g, 0, 8); return 0; }
    if (which == 4) { bitbuffer(3) u8 h = bitbuffer.from(&store[0], 8); bitbuffer.push(h, 9); return 0; }
    if (which == 5) return bitslice.get(v, i);
    if (which == 6) { bitslice.set(v, i, 0); return 0; }
    if (which == 7) { bitslice.set(v, 0, 8); return 0; }
    return 0;
}
