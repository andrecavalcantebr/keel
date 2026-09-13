/* gen/ag2/ag2.c — gerado de ag2/ag2.k, perfil C11.
   §5.10: coseq baixa para um `switch` sobre variável de etapa local — e aqui o
   `switch` é legítimo, porque o bloco só contém chamadas.
   §5.11: copar é uma sequência de blocos guardados mais duas contagens. */
#include "ag2/ag2.h"

#line 9 "ag2/ag2.k"
static keel_corot_i32 ag2_ola      (ag2_Ag *g) { if (g->a-- > 0) return (keel_corot_i32){ 0, 0 }; return (keel_corot_i32){ -1, 10 }; }
static keel_corot_i32 ag2_autentica(ag2_Ag *g) { if (g->b-- > 0) return (keel_corot_i32){ 0, 0 }; return (keel_corot_i32){ -1, 20 }; }
static keel_corot_i32 ag2_pronto   (ag2_Ag *g) { if (g->c-- > 0) return (keel_corot_i32){ 0, 0 }; return (keel_corot_i32){ -1, 30 }; }
static keel_corot_i32 ag2_quebra   (ag2_Ag *g) { (void)g; return (keel_corot_i32){ 9, 0 }; }

#line 16 "ag2/ag2.k"
keel_outcome_i32 ag2_cadeia(ag2_Ag *g) {
    keel_outcome_i32 passo = { 0, 0 };
    {   unsigned char keel__et = 0;
        for (;;) {
            switch (keel__et) {
            case 0: {
                keel_corot_i32 keel__r = ag2_ola(g);
                if (keel__r.code > 0) { passo = (keel_outcome_i32){ 1, 0 }; goto keel__passo_fim; }
                if (keel__r.code == 0) continue;            /* ONGOING: repete a etapa */
                keel__et = 1; continue;
            }
            case 1: {
                keel_corot_i32 keel__r = ag2_autentica(g);
                if (keel__r.code > 0) { passo = (keel_outcome_i32){ 2, 0 }; goto keel__passo_fim; }
                if (keel__r.code == 0) continue;
                keel__et = 2; continue;
            }
            case 2: {
                keel_corot_i32 keel__r = ag2_pronto(g);
                if (keel__r.code > 0) { passo = (keel_outcome_i32){ 3, 0 }; goto keel__passo_fim; }
                if (keel__r.code == 0) continue;
                passo = (keel_outcome_i32){ 0, keel__r.v };  /* última etapa: OK com o valor */
                goto keel__passo_fim;
            }
            default: passo = (keel_outcome_i32){ 1, 0 }; goto keel__passo_fim;
            }
        }
        keel__passo_fim: ;
    }
    return passo;
}

#line 21 "ag2/ag2.k"
keel_outcome_i32 ag2_cadeia_quebrada(ag2_Ag *g) {
    keel_outcome_i32 passo = { 0, 0 };
    {   unsigned char keel__et = 0;
        for (;;) {
            switch (keel__et) {
            case 0: {
                keel_corot_i32 keel__r = ag2_ola(g);
                if (keel__r.code > 0) { passo = (keel_outcome_i32){ 1, 0 }; goto keel__passo_fim; }
                if (keel__r.code == 0) continue;
                keel__et = 1; continue;
            }
            case 1: {
                keel_corot_i32 keel__r = ag2_quebra(g);
                if (keel__r.code > 0) { passo = (keel_outcome_i32){ 2, 0 }; goto keel__passo_fim; }
                if (keel__r.code == 0) continue;
                keel__et = 2; continue;
            }
            case 2: {
                keel_corot_i32 keel__r = ag2_pronto(g);
                if (keel__r.code > 0) { passo = (keel_outcome_i32){ 3, 0 }; goto keel__passo_fim; }
                if (keel__r.code == 0) continue;
                passo = (keel_outcome_i32){ 0, keel__r.v };
                goto keel__passo_fim;
            }
            default: passo = (keel_outcome_i32){ 1, 0 }; goto keel__passo_fim;
            }
        }
        keel__passo_fim: ;
    }
    return passo;
}

#line 28 "ag2/ag2.k"
keel_outcome_i32 ag2_juntos(ag2_Ag *g) {
    keel_outcome_i32 coleta = { 0, 0 };
    {   keel_corot_i32 keel__p = {0}, keel__t = {0};      /* {0} == ONGOING */
        size_t keel__ok = 0;
        for (;;) {
            if (keel__p.code == 0) {
                keel__p = ag2_ola(g);
                if (keel__p.code > 0) { coleta = (keel_outcome_i32){ 1, 0 }; goto keel__coleta_fim; }
                if (keel__p.code < 0) keel__ok++;
            }
            if (keel__t.code == 0) {
                keel__t = ag2_autentica(g);
                if (keel__t.code > 0) { coleta = (keel_outcome_i32){ 2, 0 }; goto keel__coleta_fim; }
                if (keel__t.code < 0) keel__ok++;
            }
            if (keel__ok >= 1) goto keel__coleta_fim;      /* ANY */
        }
        keel__coleta_fim: ;
    }
    return coleta;
}
