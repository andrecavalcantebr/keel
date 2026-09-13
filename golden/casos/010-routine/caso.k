module ag2;
import keel.corot   as corot   types;
import keel.outcome as outcome types;
import_c <stdio.h>;

pub typedef struct { int a, b, c; } Ag;

/* Participantes: cedem enquanto o contador é positivo, depois terminam. */
priv corot i32 ola     (Ag *g) { if (g->a-- > 0) coagain(0); cowin(10); }
priv corot i32 autentica(Ag *g) { if (g->b-- > 0) coagain(0); cowin(20); }
priv corot i32 pronto  (Ag *g) { if (g->c-- > 0) coagain(0); cowin(30); }
priv corot i32 quebra  (Ag *g) { cofail(9); }

/* coseq: política de avanço em SEQUÊNCIA. ONGOING repete a mesma etapa; o
   código de falha é o ordinal da etapa, base um (§4.8). */
pub outcome i32 cadeia(Ag *g) {
    coseq passo { ola(g); autentica(g); pronto(g); }
    return passo;
}

pub outcome i32 cadeia_quebrada(Ag *g) {
    coseq passo { ola(g); quebra(g); pronto(g); }
    return passo;
}

/* copar: política de avanço em PARALELO cooperativo. Bloco guardado por
   participante, e ANY sai na primeira vitória. */
pub outcome i32 juntos(Ag *g) {
    copar coleta ANY { ola(g); autentica(g); }
    return coleta;
}
