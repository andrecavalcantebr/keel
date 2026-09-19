/*
 * DESC: keel tagged — a value labelled by a declared set (spec §5.4, backend §5.2, §5.6)
 * AUTHOR: andre cavalcante
 * DATE: Sep, 2026
 * LICENSE: GPL v3 + Keel Base Library Exception — see /LICENSE.md
 */

module keel.tagged tags E type T;

// `void` drops the value field and the verbs that mention it (spec §4.3):
// `tagged Cycle void` is the label alone, with no special handling here.
pub modifier tagged { i32 tag; T value; }

// reading gives `i32` and writing takes `E`, and the asymmetry is the
// contract (spec §5.4): the stored label may not belong to the set — that is
// what the debug check observes — so `tag` hands over what is there, while
// the typed parameter is what demands membership on the way in.
pub inline i32  tag  (const tagged *t) { return t->tag; }
pub inline T    value(const tagged *t) { return t->value; }

pub inline void mark (tagged *t, E e) { t->tag = e; }
pub inline void set  (tagged *t, E e, T v) { t->tag = e; t->value = v; }
