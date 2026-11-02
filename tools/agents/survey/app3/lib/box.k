module app3.lib.box type T;

pub modifier stack byref {
    size_t cap, len;
    T *ptr;
}

pub inline size_t length(stack *s) { return s->len; }

pub bool push(stack *s, T v) {
    if (s->len == s->cap) return false;
    s->ptr[s->len++] = v;
    return true;
}
