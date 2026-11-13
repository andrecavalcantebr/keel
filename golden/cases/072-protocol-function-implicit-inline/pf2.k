module pf2;

/* Mesma forma do golden 065 (`box`, `first`/`limit`), mas `span` SEM
   `inline`: spec §4.14 regra 8 (2026-11-13) — é sempre inline, a palavra é
   redundante quando presente e dispensável quando ausente. */
pub typedef struct { i32 lo, hi; } box;

pub inline i32 first(box *x) { return x->lo; }
pub inline i32 limit(box *x) { return x->hi; }

pub i32 span(Countable items) { return limit(items) - first(items); }

pub i32 span_of(box b) { return span(b); }
