module ag2;
import keel.corot   as corot   types;
import keel.outcome as outcome types;
import keel.routine as routine types;
import keel.slice   as slice   types;

pub typedef struct { i32 a, b, c; } Ag;

priv corot ola(Ag *g)       { corot r = {0}; if (g->a-- > 0) return corot.again(r); return corot.win(r); }
priv corot autentica(Ag *g) { corot r = {0}; if (g->b-- > 0) return corot.again(r); return corot.win(r); }
priv corot pronto(Ag *g)    { corot r = {0}; if (g->c-- > 0) return corot.again(r); return corot.win(r); }
priv corot quebra(Ag *g)    { corot r = {0}; return corot.fault(r, 9); }

/* A composição é chamada de função: o que antes ficava depois de um rótulo de
   saída é o statement seguinte à declaração do resultado (§5.6). */
pub outcome u32 cadeia(Ag *g) {
    array routine.slot Ag passos[3] = {
        { .f = ola,       .ctx = g },
        { .f = autentica, .ctx = g },
        { .f = pronto,    .ctx = g },
    };
    return routine.seq(slice.of(passos));
}

/* FAILED encerra a sequência: a terceira etapa não é chamada, e seu estado
   permanece ONGOING na tabela. */
pub outcome u32 cadeia_quebrada(Ag *g, i32 *estado_terceiro) {
    array routine.slot Ag passos[3] = {
        { .f = ola,    .ctx = g },
        { .f = quebra, .ctx = g },
        { .f = pronto, .ctx = g },
    };
    outcome u32 r = routine.seq(slice.of(passos));
    *estado_terceiro = routine.code(passos[2]);
    return r;
}

/* Alvo um entre duas: a política é avaliada ao fim do ciclo, então as duas são
   chamadas na rodada em que a primeira vence. */
pub outcome u32 juntos(Ag *g) {
    array routine.slot Ag passos[2] = {
        { .f = ola,       .ctx = g },
        { .f = autentica, .ctx = g },
    };
    return routine.par(slice.of(passos), 1);
}

/* Alvo dois entre três, com uma falha: ainda é possível. */
pub outcome u32 dois(Ag *g) {
    array routine.slot Ag passos[3] = {
        { .f = quebra,    .ctx = g },
        { .f = ola,       .ctx = g },
        { .f = autentica, .ctx = g },
    };
    return routine.par(slice.of(passos), 2);
}

/* Duas falhas entre três tornam o alvo dois impossível. */
pub outcome u32 impossivel(Ag *g) {
    array routine.slot Ag passos[3] = {
        { .f = quebra, .ctx = g },
        { .f = quebra, .ctx = g },
        { .f = pronto, .ctx = g },
    };
    return routine.par(slice.of(passos), 2);
}
