# T10 — inventário do prefixo `keel_` / `KEEL_`

## Objetivo

O André vai decidir se os módulos da base (`module keel`, `module keel.<nome>`)
ficam isentos de `define-over-keel-name`, e o que fazer com a palavra v1
`keel_code`, que começa pelo prefixo reservado. Esta tarefa levanta **os
fatos**: onde e como o prefixo é usado e regrado hoje. **Não propõe a decisão
e não edita nada.**

## Arquivos

- Criar: `tools/agents/reports/T10-keel-prefix-audit.md`.
- Só leitura em todo o resto. Não rode `make`, `verify.sh` nem `golden/run.sh`.

## O que levantar

1. **As regras nos normativos.** Cite literalmente, com `arquivo:linha`, todo
   trecho que regra o prefixo: reserva, `define-over-keel-name`,
   `define-over-keel-word`, `keel-name-shadowed`, `KEEL_CHECKS`, `keel_code`,
   e qualquer exceção ou isenção. Procure em `keel-spec.md` (§2.2, §2.5, §4.1,
   §4.13, §6.2), `keel-c-backend.md` (§2.3, §4.2, §5.17, §8) e
   `cgen-tool-spec.md` (`--checks`). Também o `keel-rationale.md`, se houver
   justificativa.
2. **O uso na base.** Em `base/**/*.k`, liste cada ocorrência de
   identificador que comece por `keel_`, `KEEL_` ou `keel__`, e classifique:
   (a) `#define`/`#undef`/`#ifndef`/`#if` de macro; (b) declaração de nome
   (função, tipo, variável); (c) uso de nome gerado (chamada ou referência a
   símbolo manglado, como `keel_arena_alloc3`); (d) outro. Para (a), diga em
   qual módulo e linha, e se o macro é definido ou só testado.
3. **O código do cgen.** Em `tools/cgen/src/engine/lexer*.c` e onde mais o
   `define-over-keel-name` for emitido (`grep -rn DEFINE_OVER_KEEL_NAME
   tools/cgen/src`), descreva a isenção implementada: qual condição a liga,
   com o trecho de código colado e `arquivo:linha`. Diga se `keel_code` está
   na lista de palavras contextuais do `lexer.c`.
4. **Os testes.** Há caso de falha ou de unidade que afirme a isenção, ou que
   afirme o erro no fonte do usuário? Liste com caminho (`test/cgen/`,
   `golden/cases/`).
5. **O C gerado.** No golden (`golden/c23/`, `golden/c11/`), liste os
   `#define`/`#ifndef` de `KEEL_*` e de que `.k` e linha vêm (pelo `#line`).
6. **Usos fora da base.** Algum `.k` de `golden/cases/` escreve identificador
   `keel_`/`KEEL_` diretamente? Liste.

## Aceitação

O relatório tem as seis seções acima, cada fato com `arquivo:linha` e citação
literal curta. Onde não houver nada, escreva "nenhuma ocorrência" e o comando
`grep` que usou.

## Relatório

`tools/agents/reports/T10-keel-prefix-audit.md`, em pt-BR, com as seis seções
e, ao fim, "Incoerências vistas": contradições entre normativo, código e
testes (só anote). A mensagem final é um resumo de no máximo 10 linhas.
