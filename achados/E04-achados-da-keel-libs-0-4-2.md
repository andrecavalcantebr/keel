# E04 — achados da keel_libs fechados na 0.4.2

**Fechado em 2026-11-27 (0.4.2).** Origem: `../keel_libs/Achados/keel_achados.md`, com os repros em `rt041/`. Todos reproduziram no cgen 0.4.1. Cada um tem teste em `test/cgen/` (cli, diag ou golden).

| Achado | O que era | Resolução |
|---|---|---|
| A1 | função de módulo com dois modificadores entrava nas duas instâncias | `generic.c` passa a olhar o retorno de `k_em_other_modifier` |
| A2 | `-Wno-<nome>` e `-Werror` não valiam para os diagnósticos do keel | filtro em `diag.c` e `main.c` |
| A3 | `m *t` (modificador com `type`, sem argumento) passava como C cru | `modifier-argument-count` (spec §4.3, §6.2) |
| A4, A5 | binder `const T *p` do `walk` lido como dois declaradores | `parser_islands_types.c` |
| A6 | `type T` não lido no corpo gerava aviso do `cc` | `(void)keel__T_size/align` só do que o corpo não lê |
| A7 | `dim` decimal acima de 9 dígitos recusado | até 18 dígitos; a mensagem de `nonconstant-dim` nomeia o limite |
| A8 | `implementation-limit` de instância sem posição | aponta o primeiro uso no `.k` |
| N1 | `typedef struct { T … } X` em módulo genérico competia com `modifier` | `typedef-aggregate-of-binder` (spec §4.3 regra 15, rationale "Por que typedef não define tipo genérico"); golden 059 usa `typedef T Elem` |
| N2 | alvo escrito como `typedef buffer u8 text` não inferia a instância (`from-without-target`) | `target_is_instance` em `parser_islands_types.c`; a instância continua `keel_buffer_u8`; teste `test/cgen/cli/alias_target.sh` |
