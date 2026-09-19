module coll type T;

/* Mentions neither `T` nor `stack` → emitted ONCE, for the module, and the
   name does not carry the argument (§4.9). Same case as `outcome.OK`. */
pub constexpr i32 EMPTY = -1;

pub modifier stack byref { size_t cap, len; T *ptr; }

/* Mentions `stack`, though it never writes `T` → belongs to the instance. */
pub inline size_t length(stack *s) { return s->len; }

/* `pub` without `inline` → comes out `extern` in the instance, and the body needs an owner. */
pub bool push(stack *s, T v) {
    if (s->len == s->cap) return false;
    s->ptr[s->len++] = v;
    return true;
}
