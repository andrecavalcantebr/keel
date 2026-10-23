# T11 — relatório: a base é isenta de `reserved-name`

## O diff

- `tools/cgen/src/engine/parser_islands.c`: função nova `isl_module_in_base`,
  logo depois de `isl_module_named`. Devolve verdadeiro quando o nó `module`
  é de fato `K_AST_MODULE` e o primeiro token do nome é `keel`. Assim `keel` e
  `keel.<nome>` são base, e `keelx` ou `mykeel.a` não são. A checagem
  `kind == K_AST_MODULE` cobre o caso de `a->module` apontar para um nó que não
  é módulo (`ast.c:106` deixa `a->module = 0`).
- `tools/cgen/src/engine/parser_islands_internal.h`: declaração de
  `isl_module_in_base`, ao lado de `isl_module_named`.
- `tools/cgen/src/engine/parser_islands_checks.c`, em `isl_check_name`: o
  `reserved-name` só sai quando o nome começa com `keel_`/`KEEL_` **e** o
  módulo não é da base. O resto da função (palavras contextuais,
  `keel-name-shadowed`, `shadowed-injected-name`) não mudou.
- `tools/cgen/test/diag/cases.txt`: duas linhas novas.
- `tools/cgen/test/diag/base-names/keel/probe.k` (novo): `module keel.probe;`
  com `pub int keel_count;` e `#define KEEL_PROBE 1`, sem marcador. O caso
  afirma que a base não recebe diagnóstico.
- `tools/cgen/test/diag/base-names-keelx/keelx/q.k` (novo): `module keelx.q;`
  com `pub int keel_count;  /* DIAG: reserved-name */`. É o controle negativo:
  `keelx` não é base, e o erro continua valendo.

Não foram tocados: os normativos (`keel-spec.md` e `keel-c-backend.md` seguem
com as alterações do orquestrador, não revertidas), o golden, os casos
existentes e o `warnings.baseline`.

## Um desvio do cartão: dois diretórios de caso

O cartão pediu um caso em `diag/base-names/`. Num primeiro momento coloquei o
controle `keelx` no mesmo diretório. Dois fatos do runner (`diag.py`) fizeram
isso falhar:

1. O `expected()` varre todos os `.k` da raiz do caso, então cada linha de
   `cases.txt` exige os marcadores de **todos** os arquivos do diretório. Um
   arquivo sem marcador dentro do mesmo diretório quebra as outras linhas.
   Por isso o controle foi para `base-names-keelx/`, em raiz própria.
2. Um erro do lexer para o parser do arquivo. No `keelx/q.k` com um
   `#define KEEL_X 1` (lexer: `define-over-keel-name`), o `reserved-name` da
   linha 2 não saía. Medido: sem o `#define`, o `reserved-name` aparece. Esse
   comportamento já existia antes da mudança, e não foi alterado. Vale ao
   escrever casos: um arquivo com erro de lexer não é bom para afirmar um erro
   de parser. Por isso o `define-over-keel-name` da `keelx` não entrou no caso.

## Saídas da aceitação

`sh tools/agents/verify.sh` (rodado em segundo plano; levou mais de 120 s):

```
ok   build (4 warnings, baseline 4)
ok   make check (45 lines ok)
ok   golden: 46 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
ALL OK
```

`make -C tools/cgen check 2>&1 | tail -5`:

```
ok     cli/vector_limits ok
ok     lex_dump         ok     cgen --stop-after=lex        (cgen design §5.1)
ok     diag             45 failure cases; the catalog is covered 136/148
ok     parse_dump/ilha  ok     cgen --stop-after=parse      (cgen design §5.2, level ilha, 0 wip)
make: Saindo do diretório '/home/andre/code/cprojects/keel/tools/cgen'
```

`golden/run.sh 2>&1 | tail -1`: `46 ok, 0 fail, 0 xfail, 0 xpass, 3 wip`

`git status -s`:

```
 M keel-c-backend.md            (orquestrador, anterior a este cartão)
 M keel-spec.md                 (orquestrador, anterior a este cartão)
 M tools/cgen/src/engine/parser_islands.c
 M tools/cgen/src/engine/parser_islands_checks.c
 M tools/cgen/src/engine/parser_islands_internal.h
 M tools/cgen/test/diag/cases.txt
?? tools/agents/reports/T10-keel-prefix-audit.md     (anterior)
?? tools/agents/tasks/T10-keel-prefix-audit.md       (anterior)
?? tools/agents/tasks/T11-base-reserved-name.md      (este cartão)
?? tools/cgen/test/diag/base-names-keelx/            (novo)
?? tools/cgen/test/diag/base-names/                  (novo)
```

## As duas medições do T10

Antes da mudança (reproduzido): `keel/probe.k` dava `reserved-name` na linha 2,
e `app.k` também.

Depois da mudança:

```
$ cgen --stop-after=parse keel/probe.k     (module keel.probe; pub int keel_count;)
module	keel.probe	keel/probe.k:1:1
decl	pub var	keel_count	keel_probe_keel_count	keel/probe.k:2:1
rc=0

$ cgen --stop-after=parse app.k           (module app; pub int keel_count;)
app.k:2:9: error: `keel_count` is in the space of the generated C: names starting with `keel_` or `KEEL_` are reserved [reserved-name]
rc=1
```

O `keel.probe` ficou sem diagnóstico, e o `module app` segue com o erro.

## Dúvidas abertas

- O comportamento do item 2 acima (erro de lexer suprime o parser no mesmo
  arquivo) não foi investigado a fundo: só medido. Pode ser desejado, mas
  convém registrar, porque afeta quantos erros um caso vê de uma vez.
- O cartão supunha um só diretório `base-names/`. A mudança para dois
  diretórios é uma consequência do runner, e não de uma decisão de design.
