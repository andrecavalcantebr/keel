# T18 — `pub extern_c`, `extern_c` de uma linha e comentário no `INIT` de `constexpr`

## Objetivo

Oráculo: **`golden/cases/045-extern-forms`** (escrito à mão: `app/ef.k`, `expected/c23/app/`, `expected/c11/app/`, `proof.c`). Leia o `ef.k` e todo o `expected/c23/app/` antes de tudo. Formas hoje recusadas (`implementation-limit`):

1. **`pub extern_c { … }`**: é o plano com `pub` explícito; mesma saída do plano (`.h`, na ordem do fonte, com `#line`).
2. **`extern_c { … }`, `priv extern_c { … }` e `extern_c [type_h] { … }` com `{` e `}` na mesma linha**: o conteúdo é o texto entre as chaves, **sem os brancos junto a elas**, numa linha própria:
   - plano → no `.h`: `#line 7` e `static inline int one(void) { return 1; }`;
   - `[type_h]` → no `.type.h`: `#line 9` e `typedef int T;`;
   - `priv` → no `.c`, no lugar da linha do fonte: `static int cache = 3;` (a linha do `.k` é a mesma, sem `priv extern_c {` nem `}`).
3. **Comentário dentro do `INIT` de `constexpr`**: `pub constexpr size_t K = 1 + /* two */\n    2;` e o de bloco `constexpr size_t N = 3 + // four\n        4;`. Os comentários **saem de todo o texto gerado** (como em qualquer outro trecho, `comment_edits` em `emit.c`): sob C23 a declaração fica `1 +` / `    2;`; sob C11 o `#define` e o objeto de conferência não têm o comentário (veja o esperado). Hoje `k_em_range_plain` (`emit_core.c`) recusa qualquer `/` nos intervalos entre tokens do statement; troque a recusa por **descartar o comentário** (e os brancos antes dele, quando ele termina a linha), tanto em `k_em_put_continued` quanto na cópia do `INIT` de `emit_constexpr_c11` (`emit.c`) e no objeto de conferência do bloco.

## Arquivos

Pode editar, em `tools/cgen/src/engine/emit/`: `emit.c`, `emit_core.c`, `constexpr_block.c`, `emit_internal.h`. Pode apagar o `|wip` da linha `045-extern-forms` de `test/cgen/gen/golden-cases.txt`. Pode criar `tools/agents/reports/T18-extern-forms.md`.

Não toque: normativos, `golden/**/expected/**`, nenhum `proof.c`, nenhum outro caso. Não faça commit nem `git add`. Se um teste de unit em `test/cgen/unit/` afirmar o contrário do que o cartão pede, NÃO o edite: guarde o patch fora do repositório e diga no relatório.

## Contrato

- Código existente: `supported()` em `emit.c` (`case K_AST_EXTERN_C`: hoje recusa `{` e `}` na mesma linha e o `pub` explícito; e `case K_AST_CONSTEXPR` com `k_em_range_plain`), `extern_c_type_h`/`extern_c_priv`/`extern_c_lines` e o `h_body` (T16), `emit_constexpr_c11`, `k_em_put_continued`.
- O conteúdo de uma linha só: do primeiro token depois da `{` ao último antes da `}`; os brancos de borda caem.
- Os números de linha do `#line` são os do esperado.

## Passos sugeridos

1. `sh test/cgen/gen_golden.sh`: veja o `wip 045-extern-forms`; gere a saída numa pasta temporária e compare com o esperado. **Não copie a saída para o esperado.**
2. Uma forma por vez; `make -C tools/cgen -j8` e `sh test/cgen/gen_golden.sh` inteiro a cada passo.
3. `sh tools/agents/survey/run.sh`: x01–x04, k01 e k02 devem ir a `OK`.

## Aceitação

```sh
sh test/cgen/gen_golden.sh      # 045 sai de wip; 0 fail (os outros wip, 046-052, continuam)
golden/run.sh                   # "100 ok, 0 fail, 0 xfail, 0 xpass, 3 wip"
sh tools/agents/verify.sh       # ALL OK, sem warnings novos
sh tools/agents/survey/run.sh   # x01-x04, k01, k02 com OK nos dois perfis
```

## Relatório

`tools/agents/reports/T18-extern-forms.md`, em pt-BR: o que mudou; a saída final exata dos quatro comandos; toda dúvida ou incoerência. Entregue o relatório UMA vez pelo SubagentHandback e termine.
