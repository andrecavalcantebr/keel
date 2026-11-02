// instance: app.g03.box i32
module app.g03 type T;
pub modifier box byref { /* a value */ T v; }
pub inline T get(box *b) { return b->v; }
