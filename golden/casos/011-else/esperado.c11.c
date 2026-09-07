/* gen/app/el.c — perfil C11. Emissão do keel-c-backend.md §5.12: a declaração
   sai como estava, seguida de um `if` cujo teste é o `failed` da instância —
   nunca `if (!x)`. Tudo numa linha só, pela regra 2 do §6. */
#include "keel/prelude.h"
#include "keel/keel_outcome_i32.h"
#include "keel/keel_outcome_keel_slice_i32.h"
#include <stdio.h>

static keel_outcome_i32 app_el_le(int n) {
    if (n < 0) return keel_outcome_i32_fail(7);
    if (n == 0) return keel_outcome_i32_none();
    return keel_outcome_i32_win(n * 2);
}
static keel_outcome_keel_slice_i32 app_el_vista(int n, i32 *base) {
    if (n <= 0) return keel_outcome_keel_slice_i32_none();
    return keel_outcome_keel_slice_i32_win(keel_slice_i32_from(base, (size_t)n));
}

int app_el_dobro(int n) {
    keel_outcome_i32 v = app_el_le(n); if (keel_outcome_i32_failed(v)) return -1;
    return keel_outcome_i32_value(v);
}

int app_el_dobro_log(int n) {
    keel_outcome_i32 v = app_el_le(n); if (keel_outcome_i32_failed(v)) { puts("falhou"); return -1; }
    return keel_outcome_i32_value(v);
}

size_t app_el_tamanho(int n, i32 *base) {
    keel_outcome_keel_slice_i32 s = app_el_vista(n, base); if (keel_outcome_keel_slice_i32_failed(s)) s = keel_outcome_keel_slice_i32_win(keel_slice_i32_from(base, 0));
    keel_slice_i32 sv = keel_outcome_keel_slice_i32_value(s);
    return keel_slice_i32_length(&sv);
}

int main(void) {
    i32 base[4] = {1,2,3,4};
    if (app_el_dobro(5) != 10) return 1;
    if (app_el_dobro(-1) != -1) return 2;      /* fail(7) → saída */
    if (app_el_dobro(0)  != -1) return 3;      /* NONE também é falha */
    if (app_el_dobro_log(3) != 6) return 4;
    if (app_el_tamanho(3, base) != 3) return 5;
    if (app_el_tamanho(0, base) != 0) return 6; /* default reparou o símbolo */
    puts("ok");
    return 0;
}
