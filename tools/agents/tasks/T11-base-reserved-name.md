# T11 — a base é isenta de `reserved-name`

## Objetivo

O André decidiu (2026-10-23, alternativa A do T10): os módulos `keel` e
`keel.<nome>` são a base e são donos dos nomes `keel_`/`KEEL_`. Neles não se
aplicam `define-over-keel-name` nem `reserved-name`. O texto já está na spec
§2.5 e no backend §2.3, regra 3 (não edite os dois).

O lexer já isenta o `define-over-keel-name` (`lexer.c:51`, `lexer.c:78-86`).
Falta o `reserved-name`, que é do parser e não tem isenção. Medido no T10:
`pub int keel_count;` num `module keel.probe` dá `reserved-name`; tem de não dar.

## Arquivos

- Editar: `tools/cgen/src/engine/parser_islands_checks.c`; se precisar de um
  helper novo, `tools/cgen/src/engine/parser_islands.c` e
  `tools/cgen/src/engine/parser_islands_internal.h`.
- Criar: um caso de falha novo em `tools/cgen/test/diag/<dir>/` (nome a sua
  escolha, por exemplo `base-names`) e a linha dele em
  `tools/cgen/test/diag/cases.txt`.
- Criar: `tools/agents/reports/T11-base-reserved-name.md`.
- Nada mais. Não edite normativos, golden nem os casos existentes. Não commite.

## Contrato

O ponto de emissão, `tools/cgen/src/engine/parser_islands_checks.c:132-136`:

```c
void isl_check_name(Ctx *c, KToken name, bool top, bool function) {
    KAst *a = c->ast;
    keel_slice_char none = {0};
    if (k_token_starts_with(name, "keel_") || k_token_starts_with(name, "KEEL_"))
        isl_diag3(c, K_DIAG_RESERVED_NAME, name, name, none, none);
```

A regra: o `reserved-name` sai **só** quando o módulo do arquivo não é `keel`
nem `keel.<nome>`, isto é, quando o primeiro componente do nome do módulo não é
`keel`. Atenção: `keelx` ou `mykeel.a` **não** são base; `keel` e `keel.slice`
são. O resto da função (palavras contextuais, `keel-name-shadowed`,
`shadowed-injected-name`) não muda.

O nome do módulo está no nó `module` da AST. O helper que já existe,
`tools/cgen/src/engine/parser_islands.c:257-265`, compara o nome inteiro:

```c
bool isl_module_named(const KAst *a, const char *name) {
    const KAstNode *m = isl_module_node(a);
    char buf[64]; size_t n = 0;
    for (size_t j = m->name_first; j < m->name_end; j++) {
        KToken k = isl_tok(a, j);
        ...
```

Os tokens do nome vão de `m->name_first` a `m->name_end`, com `.` como token
próprio (veja `parser_islands.c:250-254`, que troca o `.` por `_`). Basta olhar
o primeiro: `k_token_spelled(isl_tok(a, m->name_first), "keel")`. Se criar um
helper (`isl_module_in_base`, por exemplo), declare-o no
`parser_islands_internal.h` ao lado do `isl_module_named` (linha 164).

Confira se `isl_module_node` pode devolver nulo (arquivo sem `module`); se
puder, sem módulo não é base.

## O caso de teste

Leia `tools/cgen/test/diag.sh` e um caso existente (`tools/cgen/test/diag/decls/`
e `define-keel-name/`) para saber como o runner lê os marcadores
`/* DIAG: id */` e o que ele exige de um caso. O caso novo tem de afirmar:

1. Num arquivo `module keel.probe;` (no caminho `keel/probe.k` dentro do
   diretório do caso, por causa do `module-path-mismatch`): `pub int
   keel_count;` e `#define KEEL_PROBE 1` **sem** diagnóstico.
2. Se o runner exigir pelo menos um diagnóstico por caso, acrescente um erro
   deliberado que não tenha a ver com o prefixo, com o marcador.

O caso do usuário (`decls/d.k:15`, `pub int keel_count; /* DIAG: reserved-name */`)
continua passando: é o controle de que o erro segue valendo fora da base.

## Aceitação

```sh
sh tools/agents/verify.sh
make -C tools/cgen check 2>&1 | tail -5
golden/run.sh 2>&1 | tail -1
git status -s
```

Passa quando os três primeiros terminam verdes (o `check` lista `ok` no
`diag`, com o caso novo contado) e o `git status` mostra só os arquivos acima
(mais os não rastreados deste cartão e do relatório). Mostre também que a
medição do T10 mudou: o `cgen --stop-after=parse` sobre um `keel/probe.k` com
`pub int keel_count;` não dá mais `reserved-name`, e sobre `module app;` dá.

## Relatório

`tools/agents/reports/T11-base-reserved-name.md`, em pt-BR: o diff explicado,
a saída da aceitação e das duas medições, e as dúvidas.
