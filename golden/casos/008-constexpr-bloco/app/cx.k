module app.cx;
import_c <stdio.h>;

pub typedef struct { int N; int MAX; } Cfg;

pub constexpr size_t MAX = 4;          /* escopo de arquivo: leva o prefixo do módulo */

/* Escopo de bloco: sob C11 o nome do usuário viraria macro e capturaria
   `c->N`. Por isso o backend gera nome próprio e reescreve os usos (§9.2). */
pub int soma(Cfg *c) {
    constexpr size_t N = 8;
    array i32 v[N];
    int t = 0;
    foreach (auto i : 0..N) { v[i] = (i32)i; t += v[i]; }
    c->N   = (int)N;                   /* membro: NÃO é reescrito */
    c->MAX = (int)MAX;                 /* idem */
    return t + (int)sizeof(v) / (int)sizeof(i32);
}

/* Dois blocos irmãos com o mesmo nome: o ordinal os separa. */
pub int outro(void) {
    { constexpr size_t N = 2; array i32 a[N]; a[0] = 1; return (int)(sizeof(a)/sizeof(i32)); }
}

pub int main(int argc, char **argv) {
    Cfg c = {0};
    if (soma(&c) != 28 + 8) return 1;
    if (c.N != 8 || c.MAX != 4) return 2;
    if (outro() != 2) return 3;
    puts("ok");
    return 0;
}
