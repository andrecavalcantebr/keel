module app.el;
import keel.slice   as slice   types;
import keel.outcome as outcome types;
import_c <stdio.h>;

priv outcome i32 le(int n) {
    if (n < 0) return outcome.fail(7);
    if (n == 0) return outcome.none();
    return outcome.win(n * 2);
}
priv outcome slice i32 vista(int n, i32 *base) {
    if (n <= 0) return outcome.none();
    return outcome.win(slice.from(i32, base, (size_t)n));
}

/* Forma de SAÍDA: o statement entra no `if`, verbatim. */
pub int dobro(int n) {
    outcome i32 v = le(n) else return -1;
    return outcome.value(v);
}

/* Forma de SAÍDA com bloco. */
pub int dobro_log(int n) {
    outcome i32 v = le(n) else { puts("falhou"); return -1; }
    return outcome.value(v);
}

/* Forma de DEFAULT: a expressão entra no `win` e é reatribuída ao símbolo. */
pub size_t tamanho(int n, i32 *base) {
    outcome slice i32 s = vista(n, base) else slice.from(i32, base, 0);
    slice i32 sv = outcome.value(s);
    return slice.length(sv);
}
