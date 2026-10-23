# T8 — auditoria dos designs: consolidado para decisão

Fontes: `T8a-design-audit.md` (codegen, parser, lexer) e `T8b-design-audit.md`
(cgen-tool, diag, requisitos, análises). Revisados pelo orquestrador; os ids
remetem aos relatórios, onde estão as citações completas.

## 1. Correções diretas (o design diverge do normativo; nenhuma decisão nova)

| Id | Onde | O que muda |
| --- | --- | --- |
| A1 | `codegen-design.md:66`, `:452` | sai o `arena.c` ("reescrita de `alloc(a,T,n)`"). `alloc(a,T,n)` é o parâmetro `type` geral (spec §4.4, backend §5.16), e construtores e `alloc` cru são código comum da base. O único privilégio de emissão, o `= {0}` implícito (backend §5.4, regra 8; ilha `implicit-init`), vai para o `decl.c` |
| A2 | `codegen-design.md:141` | `extern_c` vai para o `.h`; `[type_h]` para o `.type.h`; só `priv extern_c` para o `.c` (backend §4.1, regra 15) |
| A3 | `codegen-design.md:131-141` | a tabela ganha diretiva de topo (`.h`) e C de topo (`.type.h`), como no backend §4.1 |
| A4, A5 | `codegen-design.md:61-72`, `:447-462` | as duas tabelas por construção ganham o backend §5.15–5.19: `extent.c` (§5.15), parâmetro `type` em `decl.c` (§5.16), `check.c` (§5.17), `binder.c` (§5.18), `protocol.c` (§5.19) |
| D1 | `codegen-design.md:230` | a fórmula do mangling diz `<M.mod>` em vez de `<módulo>`, como no backend §2.1 |
| L1 | `lexer-design.md:112-118` | a lista de palavras contextuais ganha `protocol` (spec §2.2; o `lexer.c:28` já a tem) |
| L2 | `lexer-design.md:121` | "backend §4.2" → "backend §2.3, regra 3" |
| P1 | `parser-design.md:120-125` | `KTopKind` ganha `K_TOP_PROTOCOL` (spec §2.2, `decl-protocol` em `top-decl`) |
| B1 | `cgen-tool.md:199-200` | `-o` não é ignorado sob `--stop-after`: em `lex`/`parse` nomeia a saída, com `gen` é `output-with-gen` (ferramenta §4.2, §7.1) |
| B2 | `cgen-tool.md:113` | `--parallel-lowering=openmp` sem `-fopenmp` é `lowering-unavailable` (ferramenta §4.8) |
| B3 | `cgen-tool.md:549-557` | `--checks=off` acrescenta `-DKEEL_CHECKS=0` ao `cc`; `--line=off` só muda a emissão |
| B4 | `cgen-tool.md:400` | `--main` sem função de entrada pública é `missing-entry-point` |
| B5, B6 | `cgen-tool.md` §5.2 (9 linhas) e §13 | "spec §x" que é da linguagem passa a "linguagem §x" (pela convenção do documento, "spec" é a da ferramenta) |
| B7 | `cgen-tool.md:693` | "regra 3 do backend §4.3.2" → "regra 4, e §5.8" |
| B8 | `cgen-tool.md:868-870` | sai a tolerância à "prosa nos cabeçalhos" (P5 resolvida; o golden tem só a linha de geração) |
| B9 | `diag-design.md:111` e `tools/cgen/diag-messages.md:29` | não existe `--cgen-help=<classe>`; `--help=<classe>` vai ao `cc` (ferramenta §4.1) |
| B10 | `diag-design.md:25` | as mensagens já existem (`diag-messages.md`) |
| B11 | `diag-design.md:23`, `:286` | o catálogo tem 157 ids (139 `error`, 7 `warning`, 2 `info`, 9 `debug`), não 149 e 7 |
| B12 | `diag-design.md:103-116` | o fluxo de mensagens cita o `diag-sync.py` e o lock |
| B14 | `requisitos-v0.md:9` | os passes ganham "reanálise da AST expandida (vazia na v0)" entre expansão e papéis (spec §1.3) |

## 2. Rejeitados na revisão

- **B13** (spec `:2682`, `import-clause-order`): não é contradição. A coluna
  de condição descreve a forma errada que o programa escreveu
  (`import M types as m;`); a mensagem dá a correta.
- **`lexer-design.md:10`** (exemplo do cartão): "linguagem §4.5" está certa
  (§4.5, regra 5).

## 3. Para decisão do André

1. **Isenção da base no `define-over-keel-name`** (A-D3). O lexer isenta
   `module keel`/`keel.*`, e o prelúdio de fato define `KEEL_CHECKS`
   (`keel.k:42`). Nenhum normativo diz isso: o backend §2.3 só fala do "fonte
   do usuário". Já estava pendente (entra na spec §2.5?).
2. **`keel_code` (v1) começa por `keel_`** (A-D2), prefixo reservado.
3. **Backend §8 lista 7 ids `debug`; o catálogo tem 9** (faltam
   `par-target-above-total` e `mask-above-64-slots`). Correção de normativo.
4. **`-MQ`** (B-Dú2): o design lê, a ferramenta §4.1 só lista `-MMD`, `-MD`,
   `-MF`, `-MT`. Entra na spec ou sai do design?
5. **`info` sai por padrão** (B-Dú4): decidido só no `diag-design.md` (R1).
   Sobe para a ferramenta §7?
6. **`--instance` com módulo não achado** (B-Dú5): `module-not-found` (definido
   para `import`) ou `invalid-instance`?
7. **Onde cai a baixa de `win`/`fail`/`else` antes do `defer`** (B-Dú1) na lista
   de passes do `requisitos-v0.md`.
8. **O alvo de cobertura "135"** do `diag-design.md` (B-Dú3): os 148 não-`debug`
   ou os 139 `error`?

## 4. Menores (registro, sem urgência)

- `requisitos-v0.md:13` (B15): citar a lista fechada "O que o núcleo conhece
  pelo nome" como exceção ao "verbos por protocolo".
- `cgen-tool.md`: `execvp` e `posix_spawnp` para o mesmo `cc` (B-Dú6); a tabela
  de `tools/` da §3.1 não bate com `src/tool/` hoje (B-Dú7).
- `parser-design.md`: `KSymKind` não diz onde entram os binders de módulo e o
  `parallel.control` (A-D5); a pendência Q3 sobre o `mangle.c` está superada.
