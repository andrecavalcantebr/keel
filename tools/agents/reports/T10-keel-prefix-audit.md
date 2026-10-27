# T10 — inventário do prefixo `keel_` / `KEEL_`

Levantamento de fatos, sem decisão. Nenhum arquivo do repositório foi editado;
`make`, `verify.sh` e `golden/run.sh` não foram rodados. Usei o binário já
existente `tools/cgen/cgen` em duas medições (seção "Medições"), sobre arquivos
de teste no scratchpad.

## 1. As regras nos normativos

- **`keel-spec.md:573`** (§2.5, tabela de erros léxicos): "`#define` ou `#undef`
  de nome nos espaços `keel_` e `KEEL_` | `define-over-keel-name` | `error`".
- **`keel-spec.md:574`**: "`#define` ou `#undef` de palavra contextual keel |
  `define-over-keel-word` | `warning`".
- **`keel-spec.md:576`**: "Declaração com nome de palavra contextual keel ou de
  qualificador de import do arquivo | `keel-name-shadowed` | `warning`".
- **`keel-spec.md:580`** (§2.5, prosa): "Os nomes `keel_` e `KEEL_` são o espaço
  do C gerado, e um `#define` sobre eles o altera: é `error`." Nenhuma exceção
  nessa frase. As palavras de papel ficam fora de `define-over-keel-word` e de
  `keel-name-shadowed`; os verbos de módulo também (mesma linha). A base não é
  citada.
- **`keel-spec.md:581`**: "Para os diagnósticos `define-over-keel-name` e
  `define-over-keel-word`, keel lê o nome alvo de `#define` ou `#undef`."
- **`keel-spec.md:2693`** (§6.2, catálogo): "`reserved-name` | Identificador
  declarado pelo programa nos espaços reservados `keel_` e `KEEL_` | `error` |
  Backend / compilador C | §4.2".
- **`keel-spec.md:2719`** (§6.2): "`define-over-keel-name` | `#define` ou `#undef`
  de nome `keel_` ou `KEEL_` | `error`"; **`:2720`**: "`define-over-keel-word`
  ... exceto as de papel | `warning`"; **`:2722`**: "`keel-name-shadowed` ... |
  `warning`".
- **`keel-spec.md:220`** (§2.2): "`keel_code` | **(v1)** antes do tipo de retorno
  de função, e como espécie de parâmetro seguido de `IDENT` (§4.13)". Gramática:
  `keel-spec.md:336` (`decl-function ::= [ 'keel_code' ] ...`) e `:356`
  (`param-code ::= 'keel_code' IDENT`). Molde em §4.13 (`keel-spec.md:1668`);
  `keel-spec.md:1718`: "`keel_code` como parâmetro de função que não é molde é
  `code-param-outside-mold`"; catálogo em `:2824`.
- **`keel-c-backend.md:201`** (§2.3, título "Namespaces e nomes reservados").
- **`keel-c-backend.md:211`**, regra 1: "Todo nome que o backend cria sem origem
  no fonte começa por `keel__`. ... o programa não declara nada em `keel_`".
- **`keel-c-backend.md:212`**, regra 3: "Identificadores começando com `keel_` ou
  `KEEL_` são reservados: declará-los ou defini-los no fonte do usuário é erro
  (`reserved-name`, `define-over-keel-name`). Usar os nomes do prelúdio, como
  `KEEL_CHECK` (§5.17), é permitido. `KEEL_CHECKS` chega pela linha de comando do
  compilador C, e não por `#define` no fonte."
  Esta é a única menção à diferença entre "fonte do usuário" e base; não há
  exceção para módulos da base escrita como regra.
- **`keel-c-backend.md:383`** (§4.2, título "O prelúdio: `keel.k`"); **`:418`**
  (regra 5): "O único `#if` do prelúdio é o de `KEEL_CHECKS` (§5.17)".
- **`keel-c-backend.md:1737-1740`** (§5.17, emissão): `#ifndef KEEL_CHECKS` /
  `#define KEEL_CHECKS 1` / `#if KEEL_CHECKS`. Regras em `:1756-1757`.
- **`keel-c-backend.md:1953`** (§8, "Diagnósticos deste documento"); **`:1960`**:
  "`reserved-name` (§2.3)" com severidade `error`.
- **`keel-c-backend.md:2066`** (§10, decisão D9): as verificações são chaveadas
  por `KEEL_CHECKS`. Não há §8 com essa decisão; a nota está em §10.
- **`cgen-tool-spec.md:31`**: "o prefixo reservado é do backend". **`:105`**
  (`--checks`): "`off` passa `-DKEEL_CHECKS=0`". **`:500`**: "`--checks` não está
  entre elas: ela não muda o gerado, só a chave `KEEL_CHECKS`".
- **`keel-rationale.md:1816-1825`** (macro higiênica): "`keel_code` marca um molde
  que só expande no chamador"; "Por isso parâmetro `keel_code` fora de molde é
  erro". Não há justificativa de isenção da base. `keel-rationale.md` não contém
  "reserv" sobre o prefixo; "Prelúdio e base mínima" (`:496`) trata de imports.
- Fora dos normativos, `design/lexer-design.md:121-124`: "A reserva vale para o
  fonte do usuário (backend §2.3, regra 3). Os módulos da base, os que começam por
  `module keel` ou `module keel.<nome>`, são donos dos nomes `KEEL_`". É a única
  fonte escrita da isenção da base.

## 2. O uso na base (`base/**/*.k`)

Nenhuma ocorrência de `keel__`. Todas as demais, por classe:

**(a) `#define`/`#undef`/`#ifndef`/`#if` de macro** — só em `base/keel.k`, dentro de
`pub extern_c [type_h] {` (`:41`), módulo `module keel` (`:10`):
- `base/keel.k:42` `#ifndef KEEL_CHECKS` (testa; é `#ifndef`)
- `base/keel.k:43` `#define KEEL_CHECKS 1` (define)
- `base/keel.k:45` `#if KEEL_CHECKS` (testa)
- `base/keel.k:48` `#define KEEL_CHECK(cond, id) \` (define, ramo `#if`)
- `base/keel.k:51` `#define KEEL_CHECK(cond, id) ((void)sizeof(...))` (define, ramo `#else`)

Nenhum outro `.k` da base tem diretiva com `KEEL_`/`keel_`.

**Uso de `KEEL_CHECK` (macro do prelúdio, não diretiva)** — classificado como
"outro (d)", permitido por `keel-c-backend.md:212`:
`base/keel/buffer.k:37,38,40`; `base/keel/slice.k:31,32,34`;
`base/keel/arena.k:51`; `base/keel/routine.k:55,76`; `base/keel.k:54` (dentro do
`extern_c`, texto C). `base/keel/array.k:20` é só comentário.

**(b) declaração de nome `keel_`**: `base/keel.k:53`
`static inline size_t keel_index(size_t i, size_t d) {` — dentro do
`extern_c [type_h]`, portanto texto C (ver Medições). É a única declaração.

**(c) uso de nome gerado (símbolo manglado) no código**: nenhuma ocorrência.

**(d) outro**: só comentários.
- `base/keel.k:38` ("`cgen --checks=off` passes -DKEEL_CHECKS=0"), `:63`
  ("go through keel_index above").
- `base/keel/array.k:20` ("with keel_index (backend §5.3, §5.17)").
- `base/keel/slice.k:22-23` (`keel_slice_cursor`, `keel_slice_T_cursor`).
- `base/keel/buffer.k:22` (`keel_buffer_cursor`).

## 3. O código do cgen

`define-over-keel-name` é emitido só em `tools/cgen/src/engine/lexer/lexer.c`
(`grep -rn DEFINE_OVER_KEEL_NAME tools/cgen/src`: `lexer.c:66`). Função
`check_define`:

```c
/* define-over-keel-name (lexer-design §4): ... The body is not read. The base's
   own modules own the KEEL_ names. */                       /* lexer.c:47-49 */
static void check_define(KLexer *lexer, KToken directive) {
    if (lexer->diagnostics == NULL || lexer->keel_module) return;   /* lexer.c:51 */
    ...
    if (k_token_starts_with(name, "keel_") || k_token_starts_with(name, "KEEL_")) {  /* :64 */
        ...
        k_diag_emit(lexer->diagnostics, K_DIAG_DEFINE_OVER_KEEL_NAME, name, args);  /* :66 */
        return;
    }
    ... /* palavras contextuais: K_DIAG_DEFINE_OVER_KEEL_WORD, :73 */
}
```

- **Condição da isenção:** `lexer->keel_module` (`lexer.c:51`), ligado por
  `track_leading` (`lexer.c:83`): `lexer->keel_module = k_token_spelled(t, "keel");`,
  quando as duas primeiras tokens são `module` e `keel`. Vale para `module keel` e
  para `module keel.<nome>` (a segunda token é `keel` nos dois casos). Não há
  checagem de caminho de arquivo.
- A isenção antecede **as duas** verificações de `#define` (`keel_`/`KEEL_` e
  palavra contextual) — ou seja, vale também para `#define keel_x` num módulo da
  base, não só para `KEEL_`.
- `keel_module` aparece só em `lexer.c:20, 51, 83` e `lexer.h:33`. Nenhum outro
  arquivo do `src/` consulta a isenção.
- **Fora da isenção:** a declaração com nome `keel_`/`KEEL_` não tem o teste de
  `keel_module`. `tools/cgen/src/engine/parser/parser_islands_checks.c:136-137`:
  `if (k_token_starts_with(name, "keel_") || k_token_starts_with(name, "KEEL_"))`
  `isl_diag3(c, K_DIAG_RESERVED_NAME, name, name, none, none);`. Chamada a partir
  de `parser_islands_checks.c:271` e `:302`, `parser_islands_types.c:1184` e
  `parser_islands_flow.c:1043`.
- `keel_code` está em `k_contextual_words`: `tools/cgen/src/engine/lexer/lexer.c:27`
  (`"import_c", "instance", "keel_code", "later", ...`). Sim.

## 4. Os testes

- **Isenção (lexer), positivo:** `test/cgen/lex/diag-base.k` (`module
  keel.probe;` seguido de `#ifndef KEEL_CHECKS` / `#define KEEL_CHECKS 1`):
  "nothing on stderr, exit 0", descrito em `test/cgen/lex_dump.sh:13-15`.
- **Erro no fonte do usuário, lexer:** `test/cgen/lex/diag.k:6`
  (`#define keel_x 1`) e `:7` (`#define KEEL_CHECKS 0`), esperados em
  `test/cgen/lex/diag.stderr:4-5` como `define-over-keel-name`. Palavras
  contextuais em `diag.k:8-10` (`define-over-keel-word`, `diag.stderr:6-8`).
- **Caso de diagnóstico:** `test/cgen/diag/define-keel-name/bad.k:4`
  `#define keel_code 1 /* DIAG: define-over-keel-name */ /* a keel_ name: the
  error wins */`; `:6` `#define KEEL_X 1`. `bad.k:2-3` (`foreach`, `protocol`) são
  `define-over-keel-word`; `:5` (`#define parent 1`) é papel, sem diagnóstico.
- **CLI:** `test/cgen/cli/parse_errors.sh:11` e `test/cgen/cli/imports.py:39-40`
  (`usebad`, `define-over-keel-name`).
- **`reserved-name` no usuário:** `test/cgen/diag/decls/d.k:15`
  `pub int keel_count;  /* DIAG: reserved-name */`.
- **Não há** caso que afirme a isenção de `reserved-name` para a base, nem caso
  de `module keel.*` fora de `base/` com `keel_`/`KEEL_` declarado.
- `test/cgen/diag/signatures/s.k:3-4` usa `keel_code` em função (`not-in-v0`).

## 5. O C gerado (golden)

- **`golden/c23/keel.type.h`**: `#line 42 "keel.k"` na linha 20; `:21`
  `#ifndef KEEL_CHECKS`; `:22` `#define KEEL_CHECKS 1`; `:24` `#if KEEL_CHECKS`;
  `:27` e `:30` `#define KEEL_CHECK(cond, id)` (dois ramos). Vêm de `base/keel.k:42-51`.
- **`golden/c11/keel.type.h`**: `#line 42 "keel.k"` na linha 23; `:24`
  `#ifndef KEEL_CHECKS`; `:25` `#define KEEL_CHECKS 1`; `:27` `#if KEEL_CHECKS`;
  `:30` e `:33` `#define KEEL_CHECK(cond, id)`. Mesma origem.
- **Guardas de include** (`#ifndef`/`#define` `KEEL_..._H`): em `golden/c23/keel.h:2-3`
  (`KEEL_H`), em `golden/c23/keel.type.h:2-3` (`KEEL_TYPE_H`) e em todos os headers
  de módulo de `golden/c23/keel/` e `golden/c11/keel/` (ex.:
  `golden/c23/keel/keel_array_i32.h:2-3`, `KEEL_KEEL_ARRAY_I32_H`). Não têm `#line`:
  são emitidos pelo backend, não vêm de um `.k`. Nenhuma regra normativa os
  menciona (ver Incoerências, item 6).

## 6. Usos fora da base

`grep -rn 'keel_\|KEEL_' golden/cases --include=*.k` não retorna nada: nenhum `.k`
de `golden/cases/` escreve identificador `keel_`/`KEEL_`. As ocorrências em
`golden/cases/` estão em saídas C esperadas (`golden/cases/010-routine/expected/...`,
`golden/cases/015-foreach-range/expected/...`) e em `golden/cases/015-foreach-range/proof.c`
e `NOTES`, que não são `.k`.

## Medições

Feitas com o `tools/cgen/cgen` já existente (não recompilado), `--stop-after=parse`,
arquivos em `/tmp/claude-1000/.../scratchpad/t10/`:

1. `module app;` com `pub extern_c [type_h] { static inline int keel_probe(int i)
   {...} }` e `pub int keel_count;`: saiu só `app.k:5:9: reserved-name`. O
   `keel_probe` dentro de `extern_c` não foi acusado (texto C não é varrido).
2. `keel/probe.k` com `module keel.probe;`, `pub int keel_count;`, `#define KEEL_X 1`,
   e um `extern_c` com `keel_probe`: saiu só `keel/probe.k:2:9: reserved-name`.
   **O `#define KEEL_X 1` não foi acusado**, ou seja, a isenção de `keel.*` vale
   para qualquer arquivo com esse cabeçalho, não só para `base/`. A declaração
   `keel_count` foi acusada, então a isenção não cobre `reserved-name`.

## Incoerências vistas

1. **Isenção só no lexer.** `lexer.c:51` isenta `#define` de módulo `keel*`, mas
   `parser_islands_checks.c:136-137` (`reserved-name`) não isenta. Medição 2: um
   `pub int keel_count;` em `module keel.probe` dá erro. A base passa hoje só
   porque suas declarações `keel_` estão em `extern_c` (`base/keel.k:53`), texto
   que não é varrido.
2. **A isenção não está em nenhum normativo.** `keel-spec.md:573` e `:580` dizem
   `error` sem exceção; `keel-c-backend.md:212` diz "fonte do usuário", sem
   nomear a base. A única fonte escrita é `design/lexer-design.md:121-124` e o
   comentário de `lexer.c:47-49`. A regra é por nome de módulo, não por localização
   (medição 2).
3. **`keel_code` e `define-over-keel-word`.** `keel-spec.md:574` e `:2720` mandam
   `define-over-keel-word` (warning) para toda palavra contextual exceto as de
   papel, e `keel_code` não é de papel. O código dá só `define-over-keel-name`
   (`lexer.c:64-68`, retorna antes), e o caso `bad.k:4` diz "the error wins". A
   escolha está no teste, não no normativo.
4. **Lista de palavras contextuais.** `lexer.c:24-28` tem `keel_code`;
   `design/lexer-design.md:112-118` (a lista citada no design) não tem. Os dois
   devem ser alinhados se a lista for a fonte.
5. **`keel_code` é palavra reservada no prefixo.** `keel-spec.md:220` a lista como
   palavra contextual, e `keel-c-backend.md:212` reserva o prefixo. A regra 3 fala
   de "declará-los ou defini-los", então uso como palavra não é coberto pela
   letra. Nenhum normativo diz como `keel_code` como palavra convive com o prefixo.
6. **Guardas de include `KEEL_..._H` não normatizadas.** Os headers gerados em
   `golden/c23/` e `golden/c11/` usam `KEEL_` em guardas, e nenhum normativo lista
   esse uso (a tabela de `keel-c-backend.md:212` não cita guardas). Não é conflito
   direto, mas é um `#define KEEL_` que o backend emite fora do prelúdio.
7. **Diagnóstico `reserved-name` ausente do catálogo de `keel-spec.md` §2.5.**
   Aparece em §6.2 (`keel-spec.md:2693`) e em `keel-c-backend.md:1960`, mas não
   na prosa de §2.5 (`keel-spec.md:580`), que fala só de `define-over-keel-name`.
   Coerente, só incompleto na prosa.
