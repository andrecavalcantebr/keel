# M6c — o açúcar de índice sobre contêiner: `x[i]` e `x[a..b]`

Leia antes: `.claude/skills/haiku-task/SKILL.md` (a metade do agente), `AGENTS.md`,
`tools/agents/tasks/M6-contexto.md`, e o backend `keel-c-backend.md` §5.3 ("Açúcar de
indexação") inteiro.

## Objetivo

Baixar as ilhas `index` (`x[i]` sobre instância de modificador: `*ptr(x, i)`) e
`range-index` (`x[a..b]`, `x[a..]`, `x[..b]`, `x[..]`: `as_slice(x, a, b)`), para que o caso
`013-sugar` saia pelo cgen igual aos `expected/`, nos dois perfis. O índice sobre `array`
(`array-index`) **não** é seu: é do `index.c`, e já funciona.

## Arquivos

Edita só `tools/cgen/src/engine/emit/subscript.c` (hoje um esqueleto). Cria
`tools/agents/reports/M6c-subscript.md`.

## Contrato

As ilhas do caso (do parser):

```
ilha index        b → keel_buffer_i32_ptr1                                  app/ac.k:9:5
ilha range-index  b 2..7 → keel_buffer_i32_as_slice2                        app/ac.k:27:21
ilha range-index  b 5.. → keel_buffer_i32_as_slice2 keel_buffer_i32_length  app/ac.k:28:23
ilha range-index  b ..3 → keel_buffer_i32_as_slice2                         app/ac.k:29:23
ilha range-index  b .. → keel_buffer_i32_as_slice2 keel_buffer_i32_length   app/ac.k:30:21
```

A âncora é o nome do contêiner. O detalhe traz o símbolo do verbo (e o de `length` quando a
forma aberta precisa da ponta de cima). Se o contêiner é `byref` (`buffer`), o verbo recebe
`&x` — o esperado mostra; um `slice` vai por cópia. A saída de cada linha está em
`golden/cases/013-sugar/expected/{c23,c11}/app/app_ac.c`: copie a forma dela, e confira
contra o §5.3 (regras 3 a 5 e 7). Os índices e limites são copiados como estão escritos
(`EDIT_TOKS` faz a cópia com os renomes do resto da linha).

## Passos sugeridos

1. Leia o `.k` e o `.c` esperado do 013; rode o cgen e veja a diferença (M6-contexto).
2. `k_em_handles_subscript`: aceite `index` e `range-index` cujo detalhe tenha a forma acima.
3. `k_em_edits_subscript`: as edições, linha a linha. Compile a cada passo, sem warning novo.

## Aceitação

`sh tools/agents/verify.sh` → `ALL OK`; `sh test/cgen/gen_golden.sh` não lista mais
`013-sugar` como `wip`, e continua sem `fail`.

## Relatório

`tools/agents/reports/M6c-subscript.md`: o que mudou, a saída final dos comandos, cada
divergência do esperado que você acha ser erro do golden, e toda dúvida.
