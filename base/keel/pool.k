/*
 * DESC: keel pool — fixed-capacity slot allocator, generational free (spec §5.9, backend §5.20)
 * AUTHOR: andre cavalcante
 * DATE: Nov, 2026
 * LICENSE: GPL v3 + Keel Base Library Exception — see /LICENSE.md
 */

module keel.pool type T;

import keel.entity  as entity  types;
import keel.arena   as arena   types;
import keel.outcome as outcome types;

// `byref`, like `buffer` and `bitbuffer`: three arrays of `cap` slots each,
// allocated together by `from_arena`. `top` counts the free indices still on
// the stack `free`; `cap - top` is `length` (spec §5.9).
pub modifier pool byref { T *data; u32 *gen, *free; size_t top, cap; }

// not exported: `free` and `reset` both move a slot's generation forward by
// one, saturating at UINT32_MAX instead of wrapping to zero (spec §5.9, rule
// 6) — the same shape as `keel_inclusive` saturating at SIZE_MAX. Names
// neither `T` nor `pool`, so it is shared once by every instance, not
// replicated (backend §4.3.2, rule 6: `priv inline` is private by name, not
// by linkage — cgen bug found and fixed in the same session this module
// was written: engine/emit/files.c, header_function/header_body).
priv inline void bump(u32 *gen, size_t i) {
    KEEL_CHECK(gen[i] != UINT32_MAX, "pool-generation-overflow");
    if (gen[i] != UINT32_MAX) gen[i]++;
}

// reserves the three arrays from one arena; no room leaves `p` at capacity
// zero, like `bitbuffer.make` (spec §5.9, "Casos especiais"). There is no
// `realloc`: capacity is fixed from here on.
pub inline bool from_arena(parent pool *p, parent arena *a, size_t cap) {
    *p = (pool){0};
    T   *data  = arena.alloc(a, T,   cap);
    u32 *gen   = arena.alloc(a, u32, cap);
    u32 *slots = arena.alloc(a, u32, cap);   // not named `free`: it would shadow this module's own verb
    if (!data || !gen || !slots) return false;
    for (size_t i = 0; i < cap; ++i) { gen[i] = 0; slots[i] = (u32)(cap - 1 - i); }
    p->data = data; p->gen = gen; p->free = slots; p->top = cap; p->cap = cap;
    return true;
}

// takes a slot off the free stack; `NONE` with none left (spec §5.9).
pub inline child outcome entity alloc(parent pool *p) {
    outcome entity r = {0};
    if (p->top == 0) return outcome.none(r);
    u32 idx = p->free[--p->top];
    return outcome.win(r, (entity){ idx, p->gen[idx] });
}

// `free` is one element, `reset` is the lot (spec §5.9, rule 7) — the same
// split as `arena.reset` against freeing one allocation, which arenas do not
// offer at all. `e` is `consumes`: the value itself goes invalid, not only
// what derives from it (rule 4.12 vs. `invalidates`).
pub inline void free(parent pool *p, consumes entity e) {
    bump(p->gen, e.index);
    p->free[p->top++] = e.index;
}

pub inline void reset(invalidates pool *p) {
    for (size_t i = 0; i < p->cap; ++i) { bump(p->gen, i); p->free[i] = (u32)(p->cap - 1 - i); }
    p->top = p->cap;
}

// checked in debug, like `buffer.ptr` (spec §5.9, rule 4): an index outside
// `cap`, or a generation that does not match the slot's, is `entity` that
// never came from this `pool` or no longer does.
pub inline child T *ptr(parent pool *p, entity e) {
    KEEL_CHECK(e.index < p->cap && e.gen == p->gen[e.index], "entity-generation-mismatch");
    return &p->data[e.index];
}

// checked in every build, like `buffer.at`: a copy, or `NONE` instead of the
// abort (spec §5.9, rule 4).
pub inline outcome T at(parent pool *p, entity e) {
    outcome T r = {0};
    if (e.index < p->cap && e.gen == p->gen[e.index]) return outcome.win(r, p->data[e.index]);
    return outcome.none(r);
}

pub inline size_t length  (const pool *p) { return p->cap - p->top; }
pub inline size_t capacity(const pool *p) { return p->cap; }
