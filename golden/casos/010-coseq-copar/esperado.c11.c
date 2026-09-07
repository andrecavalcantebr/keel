/* gen/ag2/ag2.c — perfil C11. Emissão de keel-c-backend.md §5.10 e §5.11. */
#include "keel/prelude.h"
#include "keel/keel_corot_i32.h"
#include "keel/keel_outcome_i32.h"
#include <stdio.h>

typedef struct { int a, b, c; } ag2_Ag;

static keel_corot_i32 ag2_ola      (ag2_Ag *g) { if (g->a-- > 0) return (keel_corot_i32){ 0, 0 }; return (keel_corot_i32){ -1, 10 }; }
static keel_corot_i32 ag2_autentica(ag2_Ag *g) { if (g->b-- > 0) return (keel_corot_i32){ 0, 0 }; return (keel_corot_i32){ -1, 20 }; }
static keel_corot_i32 ag2_pronto   (ag2_Ag *g) { if (g->c-- > 0) return (keel_corot_i32){ 0, 0 }; return (keel_corot_i32){ -1, 30 }; }
static keel_corot_i32 ag2_quebra   (ag2_Ag *g) { (void)g; return (keel_corot_i32){ 9, 0 }; }

keel_outcome_i32 ag2_cadeia(ag2_Ag *g) {
    keel_outcome_i32 passo = { 0, 0 };
    {   unsigned char keel__et = 0;
        for (;;) {
            switch (keel__et) {
            case 0: {
                keel_corot_i32 keel__r = ag2_ola(g);
                if (keel__r.code > 0) { passo = (keel_outcome_i32){ 1, 0 }; goto keel__passo_fim; }
                if (keel__r.code == 0) continue;            /* ONGOING: repete a mesma etapa */
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
                passo = (keel_outcome_i32){ 0, keel__r.v };  /* última etapa: OK com o valor dela */
                goto keel__passo_fim;
            }
            default: passo = (keel_outcome_i32){ 1, 0 }; goto keel__passo_fim;
            }
        }
        keel__passo_fim: ;
    }
    return passo;
}

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

int main(void) {
    /* coseq: três etapas, cada uma cedendo duas vezes antes de terminar */
    ag2_Ag g = { 2, 2, 2 };
    keel_outcome_i32 r = ag2_cadeia(&g);
    if (keel_outcome_i32_failed(r)) return 1;
    if (keel_outcome_i32_value(r) != 30) return 2;   /* valor da ÚLTIMA etapa */

    /* falha na etapa 2 → código 2, o ordinal base um */
    g = (ag2_Ag){ 0, 0, 0 };
    r = ag2_cadeia_quebrada(&g);
    if (!keel_outcome_i32_failed(r)) return 3;
    if (keel_outcome_i32_code(r) != 2) return 4;

    /* copar ANY: `ola` termina primeiro (b maior), sai na primeira vitória */
    g = (ag2_Ag){ 0, 5, 0 };
    r = ag2_juntos(&g);
    if (keel_outcome_i32_failed(r)) return 5;
    if (g.b != 4) return 6;      /* `autentica` foi chamada UMA vez, não mais */

    puts("ok");
    return 0;
}
