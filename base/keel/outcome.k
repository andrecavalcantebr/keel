/*
 * module keel.outcome type T
 *
 * DESC: keel outcome — resultado final com código (spec §5.5, backend §5.14)
 * AUTHOR: andre cavalcante
 * DATE: Sep, 2026
 * LICENSE: GPL v3 + Keel Base Library Exception — ver /LICENSE.md
 */

module keel.outcome type T;

pub modifier outcome { i32 code; T value; }

// não mencionam parâmetro nem modificador: saem uma vez no módulo, não por
// instância (spec §4.3, backend §4.4.1).
pub constexpr i32 OK   = 0;
pub constexpr i32 NONE = INT32_MIN;

// `void` omite o campo `value` e todo verbo que o mencione diretamente
// (spec §4.3) — win(r,v)/value(r)/value(r,v) somem sozinhos na instância
// `outcome void`, sem tratamento especial aqui.

pub inline bool failed(outcome e) { return e.code != OK; }
pub inline bool ok    (outcome e) { return e.code == OK; }
pub inline i32  code  (outcome e) { return e.code; }
pub inline T    value (outcome e) { return e.value; }
pub inline void value (outcome *r, T v) { r->value = v; }

// escritas recebem o objeto, ajustam-no e devolvem a cópia ajustada — sair
// da função é `return outcome.win(r, valor);` (spec §5.5 §4).
pub inline outcome win (outcome *r)         { r->code = OK; return *r; }
pub inline outcome win (outcome *r, T v)    { r->code = OK; r->value = v; return *r; }
pub inline outcome fail(outcome *r, i32 c)  { r->code = c; return *r; }
pub inline outcome none(outcome *r)         { r->code = NONE; return *r; }
