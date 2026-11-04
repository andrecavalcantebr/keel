# T39 — relatório (texto do agente, gravado pelo orquestrador)

Casos 059 e 060 saem byte a byte nos dois perfis. `gen_golden`: 67 ok, 0 fail, 0 wip. `golden/run.sh`: 114 ok, 3 wip (022–024). `verify.sh`: ALL OK.

Arquivos (`tools/cgen/src/engine/emit/`): `instance.c` (`carried()` aceita tipo, constante e `priv inline` do genérico; `emit_constexpr` novo; `emit_typedef` dá tag ao struct anônimo; `put_token` prefixa pelo header), `files.c` (`priv inline` no `.h` da instância), `decls.c` (renomeia tipo/constante do genérico no corpo), `uses.c` (`own_declaration`), `emit_internal.h`.

Pontos abertos:
1. Regra do "unit" (§5.10 r1): o typedef que dá nome ao header é hoje o que se chama como o último componente do módulo (`routine` em `keel.routine`). Só `keel.routine` tem isso na base. Falta decidir o texto do §5.10 r1 / §4.3 r1.
2. Código morto: o ramo `.h` de `emit_file` (emit_proto/emit_body) nunca roda, pois `k_emit_instance` manda K_EMIT_H para `k_em_instance_h`.
3. Os clientes 059/060 disparam `instance-field-access` (warning, §4.5 r2).
