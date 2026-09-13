module ag;
import keel.corot as corot types;
import_c <stdio.h>;

pub typedef struct { int s; int n; } Agente;

pub cofsm ciclo [ST1, ST2, ST3];

priv corot i32 fn1(int *a) {
    if (*a > 0) { (*a)--; coagain(0); }
    cowin(7);
}

/* Cada rótulo é um escopo: `r` vive só em ST1, e o que atravessa estados mora
   na struct do agente (§4.8). */
pub corot i32 passo(int *a, Agente *ag) {
    while (1) {
        cofsm ciclo (ag->s) {
        ST1:
            corot i32 r = fn1(a);
            if (corot.failed(r))  cofail(1);
            if (corot.ongoing(r)) coagain(0);
            ag->n = corot.value(r);
            ag->s = ST2; cobreak;
        ST2:
            if (ag->n >= 20) coagain(0);
            ag->s = ST3;
        ST3:
            cowin(ag->n);
        }
    }
}
