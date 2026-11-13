module app.ibox type T;

pub modifier box { T *values; size_t length; }

pub inline size_t length(box *b) { return b->length; }
pub inline child T *ptr(parent box *b, size_t i) { return &b->values[i]; }
