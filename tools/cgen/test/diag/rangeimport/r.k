module r;
import keel.slice as slice types;

/* the view of an `array` is the `as_slice` of keel.array: the file imports it (spec §4.5, §5.3) */
pub size_t f(void) {
    array i32 v[8];
    slice i32 a = v[2..5];                       /* DIAG: protocol-not-satisfied */
    slice i32 b = slice.of(v);                   /* DIAG: protocol-not-satisfied */
    return slice.length(a) + slice.length(b);
}
