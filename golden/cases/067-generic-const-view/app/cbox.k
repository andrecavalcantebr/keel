module app.cbox type T;
import keel.slice as slice;

pub modifier box { T *values; size_t length; }

pub inline slice.slice const T view(box *b) {
    slice.slice const T s = { b->length, b->values };
    return s;
}
