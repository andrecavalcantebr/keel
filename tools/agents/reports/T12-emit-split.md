# T12 — relatório: `engine/emit/emit.c` em módulos (refactor, sem mudança de comportamento)

## Resultado

O refactor está feito e o comportamento é o mesmo, com uma exceção deliberada
(C11 com `constexpr` no corpo de função, ver "Dúvidas", item 1). `emit.c` foi de
715 para 609 linhas. Os quatro arquivos de construção existem e estão vazios,
ligados por ganchos. Nenhum commit foi feito, e nada fora da lista do cartão foi
tocado.

## Arquivos

- **`tools/cgen/src/engine/emit/emit_internal.h`** (novo): as `#define`
  `NAME_CAP`, `LINE_CAP`, `MAX_EDITS`; `EditKind`, `Edit` e a struct `Emit`
  (com `size_t block_ordinal`, zerado em `k_emit_file`); os protótipos das
  funções `k_em_*` de `emit_core.c` e as dos quatro arquivos de construção.
- **`tools/cgen/src/engine/emit/emit_core.c`** (novo): as definições dos
  helpers que saíram de `static`: `k_em_tok`, `k_em_tok_off`, `k_em_tok_end`,
  `k_em_first_off`, `k_em_last_end`, `k_em_text_is`, `k_em_is_visibility`,
  `k_em_is_keel`, `k_em_line_start`, `k_em_line_end`, `k_em_line_at`,
  `k_em_put_bytes`, `k_em_put_mangled`, `k_em_push_edit`, `k_em_declared_at`,
  e `k_em_sort_edits` (a ordenação por inserção, tirada do fim de
  `comment_edits`). `off_of` ficou `static` aqui, por ser usada só por
  `k_em_tok_off`.
- **`array.c`, `foreach.c`, `index.c`** (novos): `k_em_edits_X` com corpo vazio
  (`(void)` nos parâmetros) e `k_em_handles_X` que responde `false`.
- **`constexpr_block.c`** (novo): `k_em_edits_constblock` e
  `k_em_handles_constblock` vazios; `k_em_pre_lines_constblock` e
  `k_em_post_lines_constblock` vazios; `k_em_supported_constblock`, que responde
  `true` em C23 e, em C11, `false` se o corpo da função tem um token
  `constexpr`.
- **`emit.c`** (editado, 715 → 609 linhas): sai tudo o que foi movido, e as
  chamadas foram trocadas para `k_em_*`. Os ganchos ficaram assim:
  1. Em `node_edits`, depois do bloco `if (!is_keel(...))`, um
     `if (n->kind == K_AST_FUNCTION)` chama as quatro `k_em_edits_*` na ordem
     pedida e depois `k_em_sort_edits(ed, antes, *cnt)`.
  2. Em `emit_line`, `k_em_pre_lines_constblock` antes da escrita e
     `k_em_post_lines_constblock` depois. O atalho que escreve a linha direto
     virou um `if/else`, e os dois ganchos envolvem os dois caminhos.
  3. Em `islands_supported`, a espécie que não é TYPE, NAME nem CALL passa se
     uma das quatro `k_em_handles_*` a aceitar, e pula a verificação da seta.
  4. Em `supported(const Emit *e)` (antes `supported(const KAst *ast)`), no
     ramo `K_AST_FUNCTION`: `if (!k_em_supported_constblock(e, n)) return false;`.
  `comment_edits` passou a chamar `k_em_sort_edits`.

Nenhum outro arquivo foi tocado: `Makefile`, testes, golden e normativos ficaram
como estavam (o `Makefile` acha os `.c` por `find`, então os novos entram sozinhos).

## Instantâneo (comportamento)

Feito com o binário de antes do refactor, antes de qualquer edição, pelo laço do
cartão (c23 e c11 × hello, shapes, tags, `--stop-after=gen`):

    $ diff -r /tmp/t12-before /tmp/t12-after && echo identical
    identical

## verify.sh

Antes do refactor (linha de base):

    ok   build (2 warnings, baseline 4)
    ok   make check (48 lines ok)
    ok   golden: 46 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
    ALL OK

Depois do refactor:

    ok   build (2 warnings, baseline 4)
    ok   make check (48 lines ok)
    ok   golden: 46 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
    ALL OK

Os dois avisos são os de `src/tool/main.c` (`f_flag`, `pedantic_names`), que já
existiam. Nenhum aviso novo vem dos arquivos de `engine/emit/`.

`wc -l tools/cgen/src/engine/emit/*.c`: `emit.c` 609 (antes 715), `emit_core.c` 98,
`array.c` 13, `foreach.c` 13, `index.c` 13, `constexpr_block.c` 32.

## Dúvidas e desvios do cartão

1. **Mudança de comportamento, só em C11.** O cartão pede que, em C11,
   `k_em_supported_constblock` responda `false` quando o corpo da função tem
   `constexpr`. Antes do refactor, esse caso era copiado verbatim e produzia C
   inválido. Verifiquei com o HEAD (`git archive` para `/tmp/t12-head`, sem tocar
   na árvore): `test/cgen/gen/constblock/cb.k` em C11 gerava `cb.c` com
   `constexpr size_t N = 8;` dentro da função. Agora sai `UNSUPPORTED`
   (`implementation-limit`, rc=2). C23 continua idêntico ao de antes (verificado:
   a saída do HEAD e a nova são iguais). `cb.k` não está em `golden-cases.txt`, e
   nenhum oráculo verde depende do caso antigo. O instantâneo do cartão não cobre
   esse caso (não estava na lista), por isso a comparação foi feita à parte.
2. **`declares_value` foi para `emit_core.c` como `k_em_declares_value`.** O cartão
   não listava a função, mas `k_em_declared_at` a usa, e copiar a regra inline
   duplicava o predicado. A função foi exportada com o prefixo `k_em_`, e o
   `emit.c` a chama.
3. **Ganchos de construção fora de `if (!is_keel)`.** O cartão diz "no ramo `value`,
   logo depois do laço que empilha os renomes". Esse laço fica dentro de
   `if (!is_keel(...))`, e o hook foi colocado logo depois desse bloco, para que
   valha também no módulo `keel`. Hoje ele não faz nada, então não há diferença de
   comportamento. Se o André preferir o hook estritamente dentro do ramo, é mudar
   uma linha.
4. **`overlaps` continua `static` em `emit.c`.** Ela não está na lista do cartão.
   Os agentes de `array`, `foreach`, `index` e `constexpr_block` vão precisar dela
   para decidir se uma edição cai no intervalo `[a, b)` da função. Ela deve ser
   promovida para `k_em_overlaps` (em `emit_core.c` e no `emit_internal.h`) antes
   de eles começarem. Não fiz agora porque o cartão mandava ficar na lista.
5. **`supported()` mudou de assinatura** (`const Emit *e` em vez de
   `const KAst *ast`), para receber `e` e chamar `k_em_supported_constblock`. A
   chamada em `k_emit_file` passou a `supported(&e)`. `islands_supported` continua
   recebendo `ast`, porque as funções `k_em_handles_*` não precisam de `e`.
6. **`emit_line` virou `if/else`.** O atalho que escrevia a linha direto e o
   caminho com edições são agora os dois ramos de um único `if`, entre os dois
   ganchos de constexpr. O comportamento é o mesmo, e o instantâneo confirma.
