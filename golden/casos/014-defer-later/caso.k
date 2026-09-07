module app.dl;
import_c <stdio.h>;

pub void anota(int v);

/* Sem lista, o corpo referencia a variável direto e lê o valor NA SAÍDA.
   Com `[now]`, a cópia é feita no registro. A diferença é semântica. */
pub void tarde(void) {
    int x = 1;
    defer anota(x);               /* lê x na saída → 3 */
    x = 2;
    x = 3;
}

pub void cedo(void) {
    int x = 1;
    defer [now int x] anota(x);   /* copia no registro → 1 */
    x = 2;
    x = 3;
}

/* `later` é a grafia explícita do padrão, e não muda nada. */
pub void tarde_escrito(void) {
    int x = 1;
    defer [later] anota(x);
    x = 7;
}
