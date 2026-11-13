module app.ibox_use;
import app.ibox types;

pub inline size_t count(IndexPtr items) { return length(items); }

pub size_t probe_count(void) {
    i32 arr[3] = {10, 20, 30};
    box i32 b = { arr, 3 };
    return count(b);
}
