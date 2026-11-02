// instance: app.g07.box i32
module app.g07 type T;
pub modifier box byref { T v; }
pub inline T get(box *b,
                 T fallback) { return b->v ? b->v : fallback; }
