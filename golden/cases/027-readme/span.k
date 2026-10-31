module span;
import keel.slice as slice types;
import keel.range as range types;
import keel.array as array;

pub i16 sum(void) {
    array i16 dados[3] = {1, 2, 3};
    slice i16 part = slice.of(dados, 1, 3);
    part[0] = 20;
    *(slice.ptr(part) + 1) = 30;

    range indices = 0..2;
    i16 total = 0;
    foreach (size_t i : indices) {
        total += part[i];
    }
    return total;
}
