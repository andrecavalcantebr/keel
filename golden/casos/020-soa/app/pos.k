module app.pos;

import keel.arena as arena types;
import keel.slice as slice types;

/* soa (spec §4.11): `x`/`y` marcados `array` viram coluna; `ativo`, `len`
   e `cap` passam como escritos — nenhum campo, verbo ou aloca por conta do
   núcleo. `pub` porque o chamador declara a instância e a passa por
   ponteiro; a marca `soa` se repete no parâmetro para que o reconhecimento
   de `p[i].campo` continue ancorado na declaração, não em rastreamento de
   tipo entre funções. */
pub soa struct position {
    array f32 x;
    array f32 y;
    bool ativo;
    size_t len, cap;
};

pub void ocupar(soa position *p, arena *a, size_t n) {
    p->x = arena.alloc(a, f32, n);
    p->y = arena.alloc(a, f32, n);
    p->cap = n;
    p->len = 0;
}

pub bool linha(soa position *p, f32 vx, f32 vy) {
    if (p->len == p->cap) return false;
    p[p->len].x = vx;
    p[p->len].y = vy;
    p->len++;
    return true;
}

/* a ponte: `slice.from` já existente (§5.3) torna a coluna crua um `slice
   f32` comum, e `foreach` funciona sem nenhum código a mais — o mesmo
   valeria para `walk`, `apply` e `partition`/`parallel`. */
pub f32 somar_x(soa position *p) {
    slice f32 xs = slice.from(f32, p->x, p->len);
    f32 total = 0;
    foreach (f32 v, size_t i : xs) total += v;
    return total;
}
