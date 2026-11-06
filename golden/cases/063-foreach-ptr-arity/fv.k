module fv type T;

pub modifier vec byref {
    size_t len;
    T *data;
}

pub inline size_t length(vec *v) { return v->len; }
pub inline child T *ptr(parent vec *v, size_t i) { return &v->data[i]; }
