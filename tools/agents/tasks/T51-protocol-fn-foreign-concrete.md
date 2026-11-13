# T51 — função sobre protocolo com tipo concreto de outro módulo

## Objetivo

Fazer o `cgen` produzir, byte a byte, o `expected/` do caso
`golden/cases/071-protocol-function-instance/` (oráculo escrito à mão no T50),
e tirar o `|wip` da sua linha em `test/cgen/gen/golden-cases.txt`.

Hoje, para o programa do 071 (`count(IndexPtr items)` em `app.ibox_use`,
chamada com `box i32`, instância do modificador de **outro** módulo genérico
`app.ibox`), o cgen aceita e emite C quebrado. Dois defeitos, em camadas:

1. **O header da instância não sai.** A chamada é reescrita para
   `app_ibox_use_count_app_ibox_box_i32`, que nenhum arquivo declara.
2. **O corpo da instância não reescreve os verbos do protocolo.**
   `length(items)` fica cru; deve virar `app_ibox_box_i32_length(&items)`.

## Arquivos

Pode editar: `tools/cgen/src/engine/parser/parser_islands_types.c`,
`tools/cgen/src/engine/parser/parser_islands.c`, e, se preciso,
`tools/cgen/src/engine/emit/*.c` e `*.h`. Edita `test/cgen/gen/golden-cases.txt`
(só tirar `|wip` do 071). Cria `tools/agents/reports/T51-protocol-fn-foreign-concrete.md`.
**Não** edite normativos, `golden/` (o oráculo é lei: se achar que está errado,
pare e diga no relatório), nem outros casos. Não commite.

## Diagnóstico já feito (confira, não confie cegamente)

- Defeito 1: em `parser_islands_types.c`, no ramo em que o tipo concreto é
  instância de modificador (procure `isl_record_seed(c, -1, fn.s,
  K_CLOSURE_PROTOCOL_FUNCTION, ast_module(c, proto_home))`, ~linha 1095), o
  módulo da semente vem de `ast_module(c, proto_home)`, que é **nulo quando a
  função é do próprio arquivo** (ele não é seu próprio import). O ramo vizinho,
  de tipo comum (~linha 1155), já resolve isso:
  ```c
  const KSymbol *fs = k_symbol_resolve(a->symbols, name);
  const KModule *proto_module = fs && fs->origin ? fs->origin : ast_module(c, proto_home);
  ```
  Aplicar o mesmo ao primeiro ramo faz o header sair (verificado em experimento).
- Defeito 2 (não resolvido): no modo protocolo de
  `k_collect_instance_islands` (`parser_islands.c` ~1129, `ctx.proto_mode`,
  `ctx.proto_own = inst->self->ast`) o coletor supõe que o módulo da função e o do
  tipo concreto são o mesmo (caso do 065) ou que a instância vem de `--instance`
  (caso de `keel.slice.of`). Aqui o tipo concreto mora em `app.ibox`, e a semente
  da instância tem de levar esse módulo e o símbolo do tipo para o coletor,
  para que `length` resolva em `app.ibox` (`app_ibox_box_i32_length`).
  `isl_proto_param` (`parser_islands_types.c` ~1521) trata só a forma com alias
  (`Proto C x`); a forma sem alias (`IndexPtr items`) passa por `proto_edits`
  em `emit/roles.c` (~linha 100-160). Descubra qual caminho reescreve o corpo
  no 065 (tipo comum) e faça o mesmo funcionar quando o tipo concreto é de outro
  módulo.

## Aceitação

```sh
make -C tools/cgen -j8 2>&1 | grep -i "warning\|error"        # nada
sh tools/agents/verify.sh                                        # ALL OK
test/run.sh > /tmp/t51.out 2>&1; tail -8 /tmp/t51.out            # 0 fail
```

e, à mão, o 071 byte a byte (os dois perfis):

```sh
for p in c23 c11; do rm -rf /tmp/t51-$p
  tools/cgen/cgen --base-dir base -I golden/cases/071-protocol-function-instance \
    --profile=$p --stop-after=gen --dest-dir /tmp/t51-$p \
    golden/cases/071-protocol-function-instance/app/ibox_use.k
  diff -r /tmp/t51-$p golden/cases/071-protocol-function-instance/expected/$p | grep -v "^Only in /tmp/t51-$p: keel"
done
```
(sem diferenças além dos arquivos do prelúdio `keel.h`/`keel.type.h`).
Se `--profile` não for o nome da opção, veja `tools/cgen/cgen --cgen-help`.

Caso extra, sem oráculo (só deve compilar e rodar): programa com
`pub inline size_t count(IndexPtr items)` sobre `keel.buffer.buffer i32`.
Corrija se quebrar, relate se for outro defeito.

## Regras do oráculo

Se o cgen e o `expected/` divergem, **a spec decide**
(`keel-spec.md` §4.14, `keel-c-backend.md` §5.19, §4.3.2), não quem escreveu.
Em dúvida, pare e pergunte no relatório. Duas dúvidas do T50 já estão em
`tools/agents/reports/T50-oracle-071.md`: o `.c` não incluir `app_ibox_box_i32.h`
(só o `.type.h`, via header da instância) e o campo e o verbo se chamarem ambos
`length`.

## Relatório

`tools/agents/reports/T51-protocol-fn-foreign-concrete.md`, pt-BR: o que mudou,
arquivo a arquivo, a saída final dos comandos de aceitação, e todo ponto aberto.
