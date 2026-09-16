module resultados;
import keel.corot as corot types;
import keel.outcome as outcome types;

pub corot estado(i32 codigo) {
    corot r = {0};
    if (codigo > 0) return corot.fault(r, codigo);
    if (codigo == 0) return corot.again(r);
    return corot.win(r);
}

pub bool falhou(i32 codigo) {
    corot r = estado(codigo);
    return corot.faulted(r);
}

pub outcome void final(bool falha, i32 codigo) {
    outcome void r = {0};
    if (falha) return outcome.fail(r, codigo);
    return outcome.win(r);
}

pub i32 reparar(i32 codigo) {
    outcome i32 origem = {0};
    outcome.fail(origem, codigo);
    outcome i32 r = origem else 5;
    return outcome.value(r);
}

/* O argumento do verbo é avaliado uma vez, e antes do cleanup: o código
   devolvido é um e o contador externo termina em dois. */
pub corot avaliar_cleanup(i32 *contador) {
    corot r = {0};
    defer ++*contador;
    return corot.fault(r, ++*contador);
}
