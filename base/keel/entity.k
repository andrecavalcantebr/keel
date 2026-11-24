/*
 * DESC: keel entity — generational reference to a pool slot (spec §5.9, backend §5.20)
 * AUTHOR: andre cavalcante
 * DATE: Nov, 2026
 * LICENSE: GPL v3 + Keel Base Library Exception — see /LICENSE.md
 */

module keel.entity;

// `entity` is a type, not a modifier: it has no element type, and the same
// `entity` identifies a slot in any `pool`, of any `type T` (spec §5.9).
// No pointer: 8 bytes, copiable by value like any ordinary struct.
pub typedef struct { u32 index, gen; } entity;
