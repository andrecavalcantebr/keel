module pf;

/* `box` é um typedef struct comum — nem genérico, nem byref — que atende
   `Countable` (spec §5.1) por `first`/`limit`. `span` é uma função sobre
   protocolo (spec §4.14) declarada no próprio módulo, fora de qualquer
   `--instance`: o caso comum, que nenhum golden exercitava. */
pub typedef struct { i32 lo, hi; } box;

pub inline i32 first(box *x) { return x->lo; }
pub inline i32 limit(box *x) { return x->hi; }

pub inline i32 span(Countable items) { return limit(items) - first(items); }

pub i32 span_of(box b) { return span(b); }
