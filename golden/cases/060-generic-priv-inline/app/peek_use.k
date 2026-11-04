module app.peek_use;
import app.peek types;

pub i32 read_i32(i32 v) {
    box i32 b;
    b.v = v;
    return get(b);
}

pub i64 read_i64(i64 v) {
    box i64 w;
    w.v = v;
    return get(w);
}
