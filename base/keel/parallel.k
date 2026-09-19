/*
 * DESC: keel parallel — the control symbol of a `parallel` block (spec §5.7, backend §5.9)
 * AUTHOR: andre cavalcante
 * DATE: Sep, 2026
 * LICENSE: GPL v3 + Keel Base Library Exception — see /LICENSE.md
 */

module keel.parallel;

// `_Atomic` is a language qualifier, and does not depend on OpenMP: there is
// no target that offers the compiler and lacks the atomic (backend §5.9).
import_c <stdatomic.h>;

// the counters and the flag live in the control symbol, never in `static`: a
// `parallel` block inside a function called twice starts zeroed both times
// (backend §5.9). The manager writes the constant fields in the initializer.
pub typedef struct {
    _Atomic u32  wins;
    _Atomic u32  fails;
    _Atomic bool flag;
    u32          workers;
    u32          target;      // 0 = ALL: the policy asks that nobody fail
} control;

// `relaxed` is enough in all of them: the flag is a hint, and seeing it late
// costs iterations, not correctness. What has to be visible to the parent is
// published by the barrier at the end of the region (backend §5.9).
pub inline bool interrupted(control *c) {
    return atomic_load_explicit(&c->flag, memory_order_relaxed);
}

pub inline u32 wins(control *c) {
    return atomic_load_explicit(&c->wins, memory_order_relaxed);
}

pub inline bool failed(control *c) {
    return atomic_load_explicit(&c->fails, memory_order_relaxed) > 0;
}

// under ALL the target is zero, and the policy is "nobody failed"; otherwise
// it is the number of wins asked for (spec §4.8).
pub inline bool ok(control *c) {
    return c->target ? wins(c) >= c->target
                     : atomic_load_explicit(&c->fails, memory_order_relaxed) == 0;
}
