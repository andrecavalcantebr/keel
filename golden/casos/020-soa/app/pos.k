module app.pos;

import keel.arena   as arena   types;
import keel.outcome as outcome types;
import keel.slice   as slice   types;

/* soa homogêneo por campo: `x` e `y` viram colunas; `ativo` é único,
   compartilhado por toda a instância (spec §4.11). Não é `pub`: nenhum
   artefato público o menciona, então mora inteiro no `.c` gerado. */
soa struct position {
    array f32 x;
    array f32 y;
    bool ativo;
};

pub outcome i32 somar_x(arena *a, size_t n) {
    outcome i32 r = {0};
    soa position p = keel.soa_from(a, n);
    if (keel.capacity(p) == 0) return outcome.fail(r, 1);

    p.ativo = true;
    for (size_t i = 0; i < n; i++)
        keel.push(p, (f32)i, (f32)i * 2.0f);

    f32 total = 0;
    slice f32 xs = keel.slice_of(p, position.x);
    foreach (f32 v, size_t i : xs) total += v;

    return outcome.win(r, (i32)total);
}
