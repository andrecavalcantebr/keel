# T36 — refatoração de `emit/emit.c` por partes (relatório)

Refatoração sem mudança de comportamento. Os cinco blocos foram feitos na ordem do cartão, e depois de cada um rodaram `make -C tools/cgen -j8`, `gen_golden.sh` e `base.sh`. Nada foi commitado nem adicionado ao índice.

## Arquivos (linhas)

| arquivo | linhas | conteúdo |
|---|---|---|
| `emit.c` | 66 | só `k_em_node_edits`, o despachante das edições de um nó |
| `lines.c` | 195 | `k_em_comment_edits`, `split_line` (static), `spanned_end` (static) + `k_em_spanned_end()`, `k_em_copy_edited`, `k_em_drop_nested`, `k_em_emit_line`, `k_em_emit_split`, `k_em_emit_range` |
| `files.c` | 307 | `EM_DASH`, `#define skip`, `type_h_body`, `h_body`, `type_h_has_content`, `h_has_content`, `raw`/`raw_slice`/`fit`, `emit_header`, `k_em_instance_h`, `k_em_instance_c`, `emit_c`, `emit_entry`, `k_emit_file` |
| `decls.c` | 318 | `k_em_is_pub_inline`, `first_of_span` (static), `k_em_decl_edits` (novo, ver abaixo), `k_em_emit_import`, `k_em_emit_proto`, `k_em_emit_extern`, `k_em_emit_constexpr_c11`, `k_em_extern_c_type_h`, `k_em_extern_c_priv`, `k_em_extern_c_lines`, `k_em_extern_c_inline`, `k_em_extern_c_text` |
| `generic.c` | 130 | `same_text`, `same_slice` (static), `k_em_module_is_generic`, `names_binder_or_modifier` (static), `k_em_over_protocol`, `k_em_generic_out`, `k_em_other_modifier`, `k_em_skip`, `k_em_uses_share` |
| `supported.c` | 188 | `island_out`, `alone_on_line`, `islands_supported` (static), `k_em_supported` |
| `emit_internal.h` | +36 | declarações dos símbolos que cruzam arquivos |

## Mudanças além da mecânica pura (todas necessárias, ou listadas)

1. **`node_edits` partida.** Os passos 1–4 (visibilidade, extent, tags, renomeações) viraram `k_em_decl_edits` em `decls.c`, que devolve `bool`. Onde o original fazia `return;` (três ocorrências, em erro de limite), agora é `return false;`, e `k_em_node_edits` faz `if (!k_em_decl_edits(...)) return;`. Assim o efeito em erro é idêntico. A ordem das edições e o `entry`/`sort` de `k_em_node_edits` ficaram iguais.
2. **`spanned_end` lido por `emit_c`.** Era `static` em `lines.c` e `emit_c` (agora em `files.c`) o lê. Foi adicionada a função `size_t k_em_spanned_end(void)`; as duas leituras em `emit_c` passaram a chamá-la. Não há outro uso.
3. **`#define skip k_em_skip` e `#define over_protocol`** ficaram nos arquivos que os usam (`skip` em `files.c` e `generic.c`; `over_protocol` em `generic.c`).
4. **Renomeações `k_em_`** só nas funções que perderam `static` por serem usadas em outro arquivo: `node_edits`, `comment_edits`, `copy_edited`, `drop_nested`, `emit_line`, `emit_split`, `emit_range`, `is_pub_inline`, `emit_import`, `emit_proto`, `emit_extern`, `emit_constexpr_c11`, `extern_c_type_h`, `extern_c_priv`, `extern_c_lines`, `extern_c_inline`, `extern_c_text`, `uses_share`, `supported`. As demais continuam `static` com o nome original.
5. **`k_em_instance_h` / `k_em_instance_c`** foram para `files.c` (montagem de arquivo de instância), e não ficaram em `emit.c`.
6. **Cabeçalhos** dos cinco arquivos novos e de `emit.c` foram escritos no molde dos outros `emit/*.c`. Comentários existentes não foram alterados.

## Comentários que citam nomes antigos (não alterados, conforme o cartão)

Os nomes abaixo continuam no texto dos comentários, mas a função agora se chama `k_em_<nome>`:
- `lines.c:94`, `lines.c:96` (`copy_edited`, `emit_range`) — comentário de `spanned_end`, que se moveu junto.
- `else.c:133` (`emit_range`), `core.c:42` (`drop_nested`, em `emit.c`), `parallel.c:30`, `:177`, `:386`, `:405` (`emit_range`), `match.c:674` (`comment_edits`).

Vale uma passada de texto (`sed` nos comentários) em outro cartão; eu não a fiz por causa da regra de não tocar comentários.

## Funções que não foram movidas

Nenhuma. Todas as funções do escopo foram movidas, e as que não couberam em um dos cinco arquivos foram para o arquivo mais próximo do seu uso (listado acima).

## Saída dos comandos de aceitação (última rodada, após o ajuste de comentário no `emit_internal.h`)

```
make -C tools/cgen -j8   -> 0 warnings
sh test/cgen/gen_golden.sh -> 65 ok, 0 fail, 0 wip
sh test/cgen/base.sh       -> 124 ok, 0 fail, 20 wip
golden/run.sh              -> 110 ok, 0 fail, 0 xfail, 0 xpass, 3 wip
sh tools/agents/verify.sh compare <snap-refactor> -> ok build; ok make check; ok golden; ok behaviour identical; ALL OK
make -C tools/cgen check   -> 53 linhas "ok", nenhuma "FAIL" (os "0 fail" e "45 failure cases" do diag são informativos)
wc -l emit.c                -> 66 (abaixo de ~400)
```

Os unit tests e o cli passaram: `unit/emit/emit` (que compila `emit.c` com todos os outros `.c` do engine) continua verde.

## Pontos em aberto

- Nenhum teste precisou de patch; nada foi guardado fora do repositório.
- `emit.c` ainda inclui `names.h` e `symtab.h` sem uso direto; deixei como estava, por "não reformatar". Pode sair em outro cartão.
- O `#define skip k_em_skip` de `files.c` não tem comentário próprio; é uma linha de alias.
