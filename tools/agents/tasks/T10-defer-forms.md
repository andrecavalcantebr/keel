# T10 — `defer` na forma que o programador escreveu

## Objetivo

Hoje `emit/defer.c` só aceita `defer` cujo corpo é **uma instrução simples, numa linha, sem
comentário, sem chaves e sem `[`**; as outras formas dão `implementation-limit` (`k_em_handles_defer`
devolve false). A spec traz `defer { close(a); close(b); }` nos exemplos (§4.6). Faça o emissor
aceitar o corpo **como foi escrito**: chaves, quebras de linha, comentários dentro do corpo e
índices (`slots[1] = 9;`) saem copiados byte a byte (menos os renames de nomes, que o
`k_em_put_range` já faz), e cada linha do corpo mapeia à sua própria linha do `.k`.

O oráculo é o caso **`golden/cases/037-defer-forms`**, escrito à mão (`app/df.k` e `expected/c23`,
`expected/c11`). Leia o `df.k` e o `expected/c23/app/app_df.c` inteiros antes de tudo: a forma
exata da saída está lá, e ela vale mais que esta descrição. `golden/run.sh` já passa neles (o
esperado compila e roda); falta o cgen produzi-los, e `test/cgen/gen_golden.sh` os mostra como
`wip`.

## Arquivos

Pode editar: `tools/cgen/src/engine/emit/defer.c`; se for inevitável, `emit.c` e `emit_internal.h`
(no mesmo diretório). Pode apagar o campo `|wip` da linha `037-defer-forms` em
`test/cgen/gen/golden-cases.txt` (só quando o caso fechar). Pode criar `tools/agents/reports/T10-defer-forms.md`.

Não toque: nenhum normativo (`keel-*.md`, `cgen-tool-spec.md`), nenhum `golden/**/expected/**`,
nenhum `proof.c`, nenhum outro caso. Não faça commit.

## Contrato

A saída esperada, por forma (todas as linhas de `df.k`; os números são do `.k`):

1. **Corpo em chaves** (`braced`, linhas 14–17). As linhas 14–17 do `.k` viram linhas em branco no
   registro. Na saída (fim natural da função) o cleanup é o corpo copiado: `#line 14`, depois as
   quatro linhas `    {`, `        app_df_add(1);`, `        app_df_add(2);`, `    }` — a primeira com a
   indentação do `defer`, as outras como estão no fonte — e `#line 19` de volta à linha do `}` da função.
2. **Instrução em várias linhas** (`spread`, linhas 22–23): igual, sem chaves; o corpo são as duas linhas
   `    app_df_add(` e `        4);`.
3. **Índice** (`indexed`, linha 28): `defer slots[1] = 9;` sai `    app_df_slots[1] = 9;` — o `[` deixa de ser recusado.
4. **Saída por `return`** (`returned`, linhas 33–36): o `return total;` vira
   `    { int keel__rv0 = app_df_total;`, depois `#line 33`, o corpo em chaves (3 linhas), e
   `    return keel__rv0; }` — sem `#line` entre o corpo e esta linha, porque o contador já a alcança
   (33+3 = 36). Se o contador **não** a alcançar, o `#line` da linha do `return` é escrito: é o
   `k_emit_at` que decide, não você.

Regras:

- Cada linha de um corpo de `n` linhas é um registro `L<linha>|<texto>` do `EDIT_LINES`, com a linha do
  `.k` consecutiva a partir da linha do `defer`. Hoje `put_cleanup` escreve um registro só, e uma
  quebra dentro do texto quebraria o formato (um registro por `\n`): parta o texto nas quebras.
- O **registro** do `defer` (o que a linha vira) cobre **todas** as linhas do statement, da palavra
  `defer` ao `;` (ou ao `}` do corpo em chaves): cada linha dele vira linha em branco (ou, com `[now]`,
  a struct de captura vai na primeira e as outras ficam em branco). Hoje `edit_registration` apaga
  `[from, semi_end)` dentro de uma só linha `[a, b)`; é preciso apagar a fatia de cada linha.
- `Defer.semi` passa a ser o **último token** do statement: o `;`, ou o `}` do bloco. No bloco não há
  `;` a acrescentar em `put_body`. `analyze` aceita `{` … `}` balanceado como corpo, e `[` e `\n` no
  corpo; comentário **dentro** do corpo é copiado como está (é C válido na cópia).
- Os tokens de dentro de um corpo de `defer` **não são saídas da função**: nenhuma varredura de
  `return`/`break`/`continue`/`goto`, de contagem de `defer` pendentes (`pending_count`, `plan_of`,
  `inner_pending`…) nem de escopo pode olhar para dentro dele. Hoje isso era vazio porque o corpo era
  uma instrução sem chaves; com chaves, um `{`/`}` do corpo mexeria nas contagens de bloco. Trate o
  intervalo `[kw, semi]` de cada `defer` como opaco nessas varreduras.
- `[now T x]` com corpo em chaves: as mesmas capturas (`keel__c<N>.x`) valem dentro do corpo.
- Comentário **depois** do `;` na linha do `defer` continua como hoje (some com a linha).
- A recusa que sobra é só o que continua sem sentido: um `defer` fora de função, um `break`/`continue`
  sem laço, `goto` cujo rótulo não está na função (as outras recusas listadas no comentário do alto
  do arquivo). Atualize esse comentário.

Código existente que você vai tocar (leia no arquivo, não confie nesta cópia):

- `analyze` (`defer.c:270`) lê o statement: o laço `for (size_t j = body; …)` recusa `{`, `}` e `[`, e
  `plain_span` (`defer.c:248`) recusa `\n` e comentário.
- `put_body` (`defer.c:458`) já copia os tokens com os intervalos entre eles, e renomeia por token.
- `put_cleanup` (`defer.c:493`): `open_record(e, kw); … put_body(…); close_record(e);`.
- `edit_registration` (`defer.c:528`) apaga o statement da linha.
- `split_line` (`emit.c:296`) consome os registros; formato descrito em `emit_internal.h:26-39`.

## Passos sugeridos

1. Rode `sh test/cgen/gen_golden.sh` e veja `wip 037-defer-forms`. Gere a saída do cgen para o 037 numa
   pasta temporária (`tools/cgen/cgen -std=c23 --base-dir base --stop-after=gen -I golden/cases/037-defer-forms --dest-dir /tmp/t10 golden/cases/037-defer-forms/app/df.k`)
   e compare com o `expected` — a diferença é o seu trabalho. **Não copie a saída para o expected.**
2. Comece por `analyze` e pelo registro (as linhas em branco), depois pelo cleanup em várias linhas,
   depois pelas varreduras opacas.
3. A cada passo: `make -C tools/cgen -j8` e `sh test/cgen/gen_golden.sh`. A lição de 2026-11-01: ao tocar
   `defer.c`, rode o `gen_golden` **inteiro**; uma varredura sem a guarda "há `defer` nesta função"
   derrubou 009, 012 e 029 (usam `break` de `match` e `parallel`, não `defer`).
4. Fora do oráculo, teste à mão e **reporte** (sem criar casos): corpo em chaves com `[now int x]`; corpo
   em chaves com um `if` e chaves aninhadas; dois `defer` em chaves na mesma função (ordem inversa);
   `defer { … }` numa função com escada (duas saídas `return`); um comentário `//` dentro do corpo; um
   `break` dentro de um laço **dentro** do corpo.

## Aceitação

Todos, nesta ordem, a partir da raiz do repositório:

```sh
sh test/cgen/gen_golden.sh      # termina com "44 ok, 0 fail, 0 wip" (037 deixa de ser wip)
golden/run.sh                   # "70 ok, 0 fail, 0 xfail, 0 xpass, 3 wip"
sh tools/agents/verify.sh       # ALL OK, sem warnings novos
```

(Os números do `gen_golden` contam o 037 como ok depois de você tirar o `|wip` da tabela.) Se algum
caso que passava deixar de passar, o defeito é seu; não altere oráculo nem baseline.

## Relatório

`tools/agents/reports/T10-defer-forms.md`, em pt-BR: o que mudou, arquivo por arquivo; a saída final
exata dos três comandos; o resultado de cada teste à mão do passo 4 (saída do cgen e se compila com
`cc -std=c11 -fsyntax-only`); e toda dúvida ou incoerência (card × código × spec).
