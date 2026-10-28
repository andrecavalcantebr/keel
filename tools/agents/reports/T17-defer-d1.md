# T17 — `defer`, degrau D1: relatório

## O que mudou

Arquivo único: `tools/cgen/src/engine/emit/defer.c` (preenchido; antes era o esqueleto vazio).
Nada mais foi editado: nem `emit.c`, `emit_internal.h`, golden, oráculos ou normativos. Não há commit.

`git status --short` ao final:

```
 M tools/cgen/src/engine/emit/defer.c
```

### `defer.c`, por partes

- **Leitura da forma** (`analyze`): `defer body;`, `defer [later] body;` e `defer [now T1 a, T2 b] body;`. A lista `[now …]` é dividida em vírgulas de profundidade 0; cada entrada é um tipo (um ou mais tokens) e um nome, que é o último token. O corpo é uma instrução só, na mesma linha do `defer`, sem `{`, `}` nem `[` (índice), sem comentário dentro da instrução.
- **Registro** (`k_em_edits_defer`, primeira passagem): para `defer` ou `defer [later]`, apaga indentação e instrução (`EDIT_DROP`), e a linha fica vazia. Para `[now]`, troca a instrução (a partir da palavra `defer`) por `struct { T1 a; T2 b; } keel__c<N> = { a, b };`, mantendo a indentação. `<N>` conta os `[now]` da função antes deste, a partir de 0.
- **Cleanup** (segunda passagem): para cada `}` da linha que fecha um bloco com `defer` pendente, sai, numa linha só, `<indent><corpo1>; <corpo2>; }`, em ordem inversa de registro. Com `[now]`, cada identificador do corpo igual a um nome capturado (não precedido de `.` nem `->`) sai `keel__c<N>.<nome>`. Os demais tokens do corpo passam por `k_em_put_range`, então os renomes do módulo (`note` vira `app_dl_note`) já saem prontos.
- **Recusa** (`k_em_handles_defer`): `false` se a função tem `return`, `break`, `continue` ou `goto` em qualquer ponto do corpo (D2 e D3); se a instrução ocupa mais de uma linha, tem comentário, corpo com chaves ou índice; se a lista não é `later`/`now` bem formada; ou se o `}` que fecha o bloco do `defer` não é o primeiro token da sua linha.

## Edições no emissor (decisão de implementação)

Ao registrar `[now]` ou `[later]`, a instrução inteira sai da linha. O emissor já tinha empilhado, para a função, as edições de renome dos nomes do módulo dentro da instrução (`note` em `defer note(x);` gera um `EDIT_RENAME` na posição de `note`). Essas edições ficam dentro de um intervalo que o `DROP` ou o `TEXT` do registro cobre, e a cópia do emissor (`copy_edited`) assume posições crescentes. Então `k_em_edits_defer` remove de `ed` as edições cujo `at` cai dentro da instrução (`drop_edits`) e gera o texto a partir dos tokens, com `k_em_put_range`, que já faz os renomes. Por isso `[`, que é o único caso em que outra construção (`index`) emitiria edição dentro do corpo, é recusado no `analyze`: o corpo com índice fica para depois.

Não encontrei nenhum problema com `e->buf`: durante o cálculo das edições, `e->n` é zero (quem o zera é `emit_line`, só na hora de copiar), então `k_em_put_*` pode montar o texto no buffer de linha, copiado depois para o scratch (`take_buf`), como pede o cartão.

## Saídas

### `sh test/cgen/gen_golden.sh` (final)

```
wip    003-alignment
wip    006-dim
wip    007-instance
8 ok, 0 fail, 3 wip
```

Antes da alteração: `7 ok, 0 fail, 4 wip` (o `014-defer-later` estava `wip`). Agora `014-defer-later` sai `ok` nos dois perfis: o oráculo compara byte a byte `expected/c23/app/app_dl.c` e `expected/c11/app/app_dl.c` (incluindo o `app_dl.h` e o `app_dl.type.h`, se houver). Os demais casos que já eram `ok` continuam `ok`.

### Compilação do gerado de 014

```
c23: cgen rc=0; gcc -std=c2x -Wall -Wextra -c  -> rc=0, sem aviso
c11: cgen rc=0; gcc -std=c11 -Wall -Wextra -c  -> rc=0, sem aviso
```

### `sh tools/agents/verify.sh` (final)

```
ok   build (2 warnings, baseline 4)
ok   make check (48 lines ok)
ok   golden: 46 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
ALL OK
```

São 2 avisos, abaixo da linha de base de 4. Não listei os avisos por arquivo: a verificação de `defer.c` foi feita tocando o arquivo e recompilando, sem nenhuma linha de aviso com `defer` no resultado.

### Casos extras, em `/tmp` (fora do repositório)

- **Recusa por `return`** (`/tmp/t17-src/app/rf.k`, com `defer note(x);` e `if (c) return 2;`): `cgen: error: app/rf.k: the emitter does not cover this module's constructions yet [implementation-limit]`, rc 2.
- **Recusa por `}` que não inicia a linha** (`/tmp/t17-src/app/rg.k`, com `defer note(x); }`): mesmo diagnóstico, rc 2.
- **Bloco aninhado com `[now]` e `[later]`** (`/tmp/t17-src/app/nest.k`): gerado em C23 e C11, compila sem aviso. O cleanup sai no `}` do bloco interno, na ordem inversa, com a indentação do `defer`:

  ```c
      {
          int z = 2;
          struct { int z; } keel__c0 = { z };

          y = 3;
          app_nest_note(y); app_nest_note(keel__c0.z); }
      y = 4;
  ```

- **Duas capturas e dois registros no mesmo bloco** (`/tmp/t17-src/app/multi.k`): gerado em C11, compila sem aviso:

  ```c
      struct { int a; int b; } keel__c0 = { a, b };

      a = 5;
      b = 6;
      app_multi_note(a); app_multi_two(keel__c0.a, keel__c0.b); }
  ```

## Dúvidas e incoerências

1. **Indentação do `}` (corrigida).** O cartão diz que o cleanup usa a indentação do `defer`, e que o `}` é o primeiro token da linha. Minha primeira versão deixava os brancos que precedem um `}` indentado na frente do cleanup (saía com 12 espaços em vez de 8). Passei a substituir também esses brancos (`EDIT_TEXT` com `skip` igual aos brancos). No golden o `}` está na coluna 0, então a mudança não o afeta, e a verificação de `gen_golden.sh` continua passando.
2. **Indentação com vários `defer` no mesmo bloco.** O cartão não diz qual indentação usar quando há mais de um `defer` pendente. Usei a do primeiro na ordem inversa, que é o último registrado. Nenhum caso do golden cobre isso; sugiro confirmar com o backend §5.5 ou o orquestrador.
3. **Corpo com índice é recusado.** O cartão não trata isso. Recusei com `[` no corpo porque o `index.c` emitiria edições dentro da instrução, que o registro reescreve. Fica para depois; não há caso no golden.
4. **Comentário dentro da instrução é recusado.** O cartão também não fala disso. Motivo: o texto do cleanup é copiado dos tokens, e um `//` dentro dele quebraria a linha de C.
5. **Erro de parser fora do escopo do cartão.** O parser já emite `K_DIAG_DEFER_IN_CONTROL_BLOCK` para `defer` dentro de `if`, `else` e `switch`, e `K_DIAG_DEFER_WITHOUT_BRACES` quando falta a chave do bloco. Não precisei tratar nada disso no emissor.
6. **Corpo com chaves (`defer { … }`).** O exemplo do backend §5.5 mostra um corpo entre chaves (`defer [now int fd, FILE *out] { report(out, fd); }`). O cartão é D1 e não o cobre; ele é recusado, e o caso aparece como `implementation-limit`. Não é um erro, mas vale saber que o exemplo do backend não está coberto por este degrau.
7. **Captura sem análise de escopo.** A troca de `a` por `keel__c<N>.a` é feita pelo nome, sem tabela de símbolos. Como o corpo é uma única instrução, sem declarações, isso só erra se o corpo tiver um nome igual a uma captura que não seja a variável capturada, o que o parser já trata como erro de escopo. Não testei esse caso.

## Verificação feita

- `make -C tools/cgen -j8`: sem aviso novo.
- `sh test/cgen/gen_golden.sh`: `8 ok, 0 fail, 3 wip` (014 agora `ok`).
- `sh tools/agents/verify.sh`: `ALL OK`.
- Compilação de 014 nos dois perfis, e dos casos extras `nest` e `multi` em C11 (e `nest` em C23), com `gcc -Wall -Wextra`, sem aviso.
- Recusas de `return` e de `}` fora do início da linha, com `implementation-limit`.
