module app.cbox_use;
import app.cbox types;
import keel.slice as slice;

pub size_t probe_length(void) {
    i32 arr[3] = {10, 20, 30};
    box i32 b = { arr, 3 };
    slice.slice const i32 v = view(b);
    return slice.length(v);
}

pub i32 probe_get(size_t i) {
    i32 arr[3] = {10, 20, 30};
    box i32 b = { arr, 3 };
    slice.slice const i32 v = view(b);
    return slice.get(v, i);
}
