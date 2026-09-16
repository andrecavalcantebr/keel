module app.cfg;

import keel.arena   as arena   types;
import keel.buffer  as buffer  types;
import keel.outcome as outcome types;
import_c <stdio.h>;

pub constexpr size_t MAX = 256;

/* Toda saída é `return` ou fim natural → escada de rótulos (backend §5.5.2). */
pub outcome i32 soma(arena *a, const char *caminho) {
    outcome i32 resultado = {0};
    FILE *ref fp = fopen(caminho, "rb");
    if (!fp) return outcome.fail(resultado, 1);
    defer [now FILE *fp] fclose(fp);

    buffer i32 xs = buffer.from(arena.alloc(a, i32, MAX), MAX);
    if (buffer.capacity(xs) == 0) return outcome.fail(resultado, 2);

    i32 v;
    while (fscanf(fp, "%d", &v) == 1)
        if (buffer.push(xs, v) == NULL) return outcome.fail(resultado, 3);

    i32 total = 0;
    foreach (i32 x, size_t i : xs) total += x;
    return outcome.win(resultado, total);
}

/* Arena no frame: `from_stack` exige tamanho constante de compilação. */
pub outcome i32 soma_scratch(const char *caminho) {
    outcome i32 resultado = {0};
    arena t;
    if (!arena.from_stack(t, 4096)) return outcome.fail(resultado, 4);
    return soma(t, caminho);
}
