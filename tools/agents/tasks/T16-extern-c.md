# T16 — WP1b: `extern_c` plano e `priv extern_c`

## Objetivo

`extern_c { … }` (sem `[type_h]`) e `priv extern_c { … }` são recusados (`implementation-limit`); só o
`extern_c [type_h]` é emitido. O backend §4.3.2, regras 9 e 10, agora diz onde cada um vai. O oráculo é o
caso **`golden/cases/043-extern-c`** (escrito à mão: `app/xc.k`, `expected/c23/app/`, `expected/c11/app/`,
`proof.c`); leia o `xc.k` e todo o `expected/c23/app/` antes de tudo:

1. **`extern_c { … }` plano** → no `.h`, **na ordem do fonte** entre os protótipos e as declarações
   `extern` vizinhos: o conteúdo (as linhas entre a `{` e a `}`), intacto, com a indentação do fonte,
   precedido de `#line` com a linha do seu primeiro caractere. A linha da `extern_c {` e a da `}` não
   saem. Os corpos `static inline` de `pub inline` continuam depois de todos os protótipos.
2. **`priv extern_c { … }`** → no `.c`, na posição do fonte: as linhas do conteúdo intactas; a linha da
   `priv extern_c {` e a da `}` viram linhas em branco. O `#line` do `.c` segue a regra geral (aparece
   quando o contador diverge).
3. O `extern_c [type_h]` não muda.

## Arquivos

Pode editar, em `tools/cgen/src/engine/emit/`: `emit.c` e `emit_internal.h`. Pode apagar o `|wip` da linha
`043-extern-c` de `test/cgen/gen/golden-cases.txt` quando fechar. Pode criar
`tools/agents/reports/T16-extern-c.md`.

Não toque: normativos, `golden/**/expected/**`, nenhum `proof.c`, nenhum outro caso. Não faça commit. Se um
teste de unit em `test/cgen/unit/` afirmar o contrário do que o cartão pede, NÃO o edite: guarde o patch
fora do repositório e diga no relatório.

## Contrato

- O conteúdo é opaco: nenhum nome dentro dele é renomeado, e as ilhas que o parser não deixa lá não
  existem. Veja como o `extern_c [type_h]` já é emitido em `type_h_body` (`emit.c`, o `case K_AST_EXTERN_C`:
  "the lines after the `{` line, up to the `}` line") e faça o análogo na ordem do fonte no `h_body`
  (hoje dois laços: protótipos, depois corpos `pub inline`) e no `emit_c`.
- No `h_body`, o laço dos protótipos já percorre os nós na ordem; o `extern_c` plano entra nesse mesmo laço,
  no ponto do seu nó.
- `supported()`: hoje o `case K_AST_EXTERN_C` exige o `[type_h]`; aceite as duas outras formas.
- A decisão de `h_has_content`, `type_h_has_content` e do corte entre os arquivos deve considerar o
  `extern_c` plano como conteúdo do `.h`.
- Linha do `.c`: no `emit_c`, as linhas de um `extern_c` plano (no `.h`) ficam em branco; as do `priv`
  ficam intactas (o conteúdo) com `{` e `}` em branco.

## Passos sugeridos

1. `sh test/cgen/gen_golden.sh`: veja o `wip 043-extern-c`; gere a saída numa pasta temporária e compare com
   o esperado. **Não copie a saída para o esperado.**
2. `make -C tools/cgen -j8` e `sh test/cgen/gen_golden.sh` inteiro a cada passo.
3. `sh tools/agents/survey/run.sh`: d08 e d09 devem ir a `OK`.

## Aceitação

```sh
sh test/cgen/gen_golden.sh      # 043 sai de wip; 0 fail
golden/run.sh                   # "84 ok, 0 fail, 0 xfail, 0 xpass, 3 wip"
sh tools/agents/verify.sh       # ALL OK, sem warnings novos
sh tools/agents/survey/run.sh   # d08 e d09 com OK nos dois perfis
```

## Relatório

`tools/agents/reports/T16-extern-c.md`, em pt-BR: o que mudou; a saída final exata dos quatro comandos; toda
dúvida ou incoerência. Entregue o relatório UMA vez pelo SubagentHandback e termine.
