/*
 * module keel.arena
 *
 * DESC: keel arena — armazenamento por região (spec §5.2, backend §5.4)
 * AUTHOR: andre cavalcante
 * DATE: Sep, 2026
 * LICENSE: GPL v3 + Keel Base Library Exception — ver /LICENSE.md
 */

module keel.arena;

// `arena` é um tipo comum, não um modificador — tem passagem por referência
// pela mesma regra dos verbos abaixo, sem que isso o torne `byref` (spec
// §4.3: "isso não o torna um modificador").
pub typedef struct { size_t top, cap; u8 *ptr; } arena;

// `from_stack(a, N)` cria armazenamento automático no escopo do chamador e
// não é redutível a uma função: `N` vira o tamanho de um vetor local. É
// C puro passando opaco por este arquivo, como qualquer macro do usuário.
#define from_stack(a, N) u8 keel__arr[N]; from_array((a), keel__arr, (N))

// `alloc(a, T, n)` lê `T` no ponto de chamada para obter `sizeof`/`_Alignof`;
// nenhuma função ordinária alcança isso para um `T` escolhido a cada uso.
#define alloc(a, T, n) ((T *)alloc_((a), sizeof(T), _Alignof(T), (n)))

pub inline bool from_array(arena *a, u8 arr[], size_t len) {
    *a = (arena){0};
    if (arr && len > 0) {
        a->cap = len;
        a->ptr = arr;
    }
    return a->cap > 0;
}

pub inline bool from_memory(arena *a, u8 *ptr, size_t len) {
    *a = (arena){0};
    if (ptr && len > 0) {
        a->cap = len;
        a->ptr = ptr;
    }
    return a->cap > 0;
}

pub inline bool from_parent(arena *a, arena *pai, size_t n) {
    *a = (arena){0};
    u8 *base = alloc(pai, u8, n);
    if (base) {
        a->cap = n;
        a->ptr = base;
    }
    return a->cap > 0;
}

// alinha o ENDEREÇO, não o deslocamento — a base pode estar em qualquer
// lugar (backend §5.4). `n * sz` transbordando devolve NULL (§5.2 §5, backend
// diagnóstico `alloc-overflow`).
priv inline void *alloc_(arena *a, size_t sz, size_t align, size_t n) {
    if (sz == 0 || n > SIZE_MAX / sz) return NULL;
    size_t need = n * sz;
    uintptr_t base = (uintptr_t)(a->ptr + a->top);
    size_t pad = (size_t)(-(uintptr_t)base & (align - 1));
    size_t livre = a->cap - a->top;
    if (pad > livre || need > livre - pad) return NULL;
    a->top += pad + need;
    return a->ptr + a->top - need;
}

pub inline size_t mark(const arena *a) { return a->top; }
pub inline void   restore(arena *a, size_t m) { if (m <= a->top) a->top = m; }
pub inline void   reset(arena *a) { a->top = 0; }
pub inline size_t length(const arena *a) { return a->top; }
pub inline size_t capacity(const arena *a) { return a->cap; }
