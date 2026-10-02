/*
 * DESC: keel protocols — the named contracts the constructions consume (spec §5.1)
 * AUTHOR: andre cavalcante
 * DATE: Oct, 2026
 * LICENSE: GPL v3 + Keel Base Library Exception — see /LICENSE.md
 */

module keel.protocols;

// A protocol emits no C. Conformance is structural: a type meets a protocol
// when its module declares the verbs (spec §5.1). Roles written here are
// inherited by an implementer that writes none.

pub protocol Countable type C {
    type index;
    index first(C *x);
    index limit(C *x);
}
pub protocol IndexPtr type C {
    type elem;
    size_t length(C *x);
    child elem *ptr(parent C *x, size_t i);
}
pub protocol IndexGet type C {
    type elem;
    size_t length(C *x);
    elem get(C *x, size_t i);
}
pub protocol Sliceable type C {
    type view;
    size_t length(C *x);
    child view as_slice(parent C *x, size_t a, size_t b);
}
pub protocol Traversable type C {
    type cursor;
    type elem;
    child cursor begin(parent C *x);
    bool has_next(C *x, cursor *c);
    child elem *next(parent C *x, cursor *c);
}
pub protocol Partitionable type C {
    type part;
    child part partition(parent C *x, size_t k, size_t w);
}
pub protocol Taggable type C {
    type tagset;
    tagset tag(C *x);
}
pub protocol Failable type C {
    bool failed(C *x);
}
pub protocol Winnable type C {
    type fallback;
    C win(C *x, fallback d);
}
