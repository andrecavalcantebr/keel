module ag;
import keel.corot  as corot  types;
import keel.tagged as tagged types;
import_c <stdio.h>;

pub tags Ciclo [ST1, ST2, ST3];

pub typedef struct { i32 n; } Agente;

priv corot fn1(i32 *a) {
    corot r = {0};
    if (*a > 0) { (*a)--; return corot.again(r); }
    return corot.win(r);
}

/* Cada braço é um escopo: `r` vive só em ST1, e o que atravessa braços mora
   na struct do agente (§4.9). O laço é de quem chama. */
pub corot passo(i32 *a, tagged Ciclo void *st, Agente *ag) {
    corot out = {0};
    match (st) {
        ST1:
            corot r = fn1(a);
            if (corot.faulted(r)) return corot.fault(out, 1);
            if (corot.ongoing(r)) return corot.again(out);
            ag->n = 7;
            tagged.mark(st, ST2);
            break;
        ST2:
            if (ag->n >= 20) return corot.again(out);
            tagged.mark(st, ST3);
        ST3:
            return corot.win(out);
    }
    return corot.again(out);
}
