// instance: app.g06.box i32
module app.g06 type T;
pub modifier box byref { T v; }
pub inline T get(box *b) {
    // the value
    return b->v;
}
