# T34 — walk com a `{` do corpo em linha própria (relatório)

## O que mudou

- `tools/cgen/src/engine/emit/foreach.c`:
  - Novo `walk_value`: monta o binder do walk (`T *p = S_next(keel__c<N>, &c);`). Antes, esse texto era montado inline em `edits_walk`; agora `edits_walk` o chama, com o mesmo resultado.
  - Novo `walk_brace_line`: a regra Allman (§5.7, regra 5) para o walk, análoga a `body_brace_line` do foreach. Quando a `{` do corpo está numa linha própria, ela recebe o binder após a chave.
  - `edits_walk`, ramo "não é a linha do cabeçalho": além de fechar com ` }`, agora chama `walk_brace_line`. Esse era o defeito: a linha da `{` não recebia a declaração do binder.
- `test/cgen/gen/golden-cases.txt`: apagado o `|wip` da linha `057-walk-allman` (o campo de flag ficou vazio, `-|`, como na linha 055).

Nada mais foi editado. Não há patch guardado: nenhum teste (unit/ ou cli/) afirmava o contrário do cartão. `test/cgen/cli/statements.sh` só confere o dump de parse do walk, sem relação com a emissão.

## Verificação

Caso 057 gerado pelo cgen (`--stop-after=gen`, dois perfis, saída em scratchpad) comparado com `golden/cases/057-walk-allman/expected/{c23,c11}/app`: idêntico byte a byte nos dois perfis. (`diff` apontou apenas `keel.h` e `keel.type.h`, arquivos que o oráculo não julga.)

Saída dos comandos de aceitação:

```
sh test/cgen/gen_golden.sh
wip    058-defer-index (differs)
64 ok, 0 fail, 1 wip        (rc 0)

golden/run.sh
110 ok, 0 fail, 0 xfail, 0 xpass, 3 wip

sh tools/agents/verify.sh
ok   build (0 warnings, baseline 0)
ok   make check (53 lines ok)
ok   golden: 110 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
ALL OK                       (rc 0)
```

O cartão pedia "0 fail" no gen_golden: confere. O `wip` restante (058) é o esperado.

## Dúvidas e pontos abertos

- `gen_golden.sh` mostra `64 ok, 1 wip` e não o total que o cartão sugere; a aceitação do cartão pede só "0 fail", então não é divergência.
- Não rodei `rm`: uma tentativa de limpar diretório temporário com variável no caminho foi bloqueada pela checagem de segurança e não foi executada. Usei diretórios novos no scratchpad.
- `tools/agents/verify.sh` grava logs em `/tmp/agents_build.$$` e `/tmp/agents_check.$$` (comportamento do script, não alterado aqui).
- Não fiz commit nem `git add`.
