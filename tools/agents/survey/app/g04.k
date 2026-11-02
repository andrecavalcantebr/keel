// instance: app.g04.stack i32
module app.g04 type T;
pub modifier stack byref {
    size_t cap, len;
    T *ptr;
}
pub inline size_t length(stack *s) { return s->len; }
