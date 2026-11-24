module app.ca;
import recs types;
import keel.slice as slice;
import keel.buffer as buffer;
import_c <stdio.h>;

pub int main(int argc, char **argv) {
    const u8 raw[3] = {1, 2, 3};
    bytes b = {3, raw};
    if (slice.length(b) != 3) return 1;
    Person ps[2] = {{40}, {41}};
    people p = {2, ps};
    if (slice.length(p) != 2) return 2;
    wide w = {0};
    if (buffer.length(w) != 0) return 3;
    puts("ok");
    return 0;
}
