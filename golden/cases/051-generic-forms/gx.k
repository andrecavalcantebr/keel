module gx type T;

pub modifier stack byref {
    size_t cap, len;
    T *ptr;
}

pub modifier cell byref { /* a value */ T v; }

priv T peek(cell *c) { return c->v; }

pub T get(cell *c) { return peek(c); }

pub inline size_t length(stack *s) { return s->len; }
