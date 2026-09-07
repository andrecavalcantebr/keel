module app.dr;
import_c <stdio.h>;

pub typedef struct { int v; } Rec;

priv void zera(Rec *r) { r->v = 0; }
priv void marca_a(Rec *r) { r->v = r->v * 10 + 1; }
priv void marca_b(Rec *r) { r->v = r->v * 10 + 2; }

/* `expr` é avaliada ANTES do cleanup, e o valor devolvido é o dela — daí o
   temporário (§4.7). Aqui o cleanup zera o campo que a expressão lê, então a
   ordem é observável: devolver 0 seria a ordem errada. */
pub int consome(Rec *r, int modo) {
    defer zera(r);
    if (modo == 0) return r->v;
    return r->v * 2;
}

/* Duas saídas e dois registros: ordem inversa, e o `return` anterior ao
   segundo registro limpa só o primeiro. */
pub int dois(Rec *r, int cedo) {
    defer marca_a(r);
    if (cedo) return -1;
    defer marca_b(r);
    return -2;
}

/* Retorno `void`: a expressão continua sendo avaliada, sem temporário. */
pub void nada(Rec *r) {
    defer zera(r);
    r->v = 9;
    return;
}
