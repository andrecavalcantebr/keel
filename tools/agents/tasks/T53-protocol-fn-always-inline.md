# T53 — função sobre protocolo é sempre inline (spec §4.14, regra 8)

## Objetivo

Fazer o cgen aceitar uma função sobre protocolo **sem a palavra `inline`** e
emitir o mesmo C que emitiria com ela — spec §4.14, regra 8 (reescrita em
2026-11-13): "A função sobre protocolo é sempre `inline`[...]; não há forma
fora de linha, porque `instance` não nomeia função". Hoje ela é aceita e o
link falha em silêncio (`undefined reference`): o protótipo sai, o corpo não.

O oráculo é `golden/cases/072-protocol-function-implicit-inline/`
(`pf2.k`, `expected/c23/`, `expected/c11/`), com `|wip` em
`test/cgen/gen/golden-cases.txt`. Objetivo: tirar o `|wip`, sem quebrar nada.

## Arquivos

Pode editar: `tools/cgen/src/engine/emit/files.c`, e, só se a investigação
abaixo mostrar necessidade real (não por precaução), `tools/cgen/src/engine/emit/instance.c`
e `uses.c`. Edita `test/cgen/gen/golden-cases.txt` (só tirar `|wip` do 072).
Cria `tools/agents/reports/T53-protocol-fn-always-inline.md`. **Não** edite
normativos, `golden/` (oráculo é lei; se achar que está errado, pare e diga),
nem outros casos. Não commite.

## Diagnóstico já feito

A causa, isolada: `files.c`, função `header_body` —

```c
/* a `pub inline` function, or in an instance a `priv inline` one: its body is in the .h */
static bool header_body(const Emit *e, const KAstNode *n) {
    return k_em_is_pub_inline(e->ast, n) || (e->inst && n->kind == K_AST_FUNCTION && n->is_inline);
}
```

Essa função decide se o CORPO de um `pub` vai para o `.h` (como
`static inline`) ou fica de fora (esperando um lugar fora de linha que, para
função sobre protocolo, nunca existe). Quando `k_emit_instance` emite o header
de uma instância de função sobre protocolo, `e->inst->proto_fn.len` é
não-zero, e o chamador (`h_body`, mesmo arquivo) já filtrou por `skip(e,n)`
antes de chegar aqui — pela própria regra de `k_em_skip` (`emit/generic.c`,
~linha 96-99), quando `e->inst->proto_fn.len` é não-zero, `skip` só deixa
passar o **próprio** nó da função sobre protocolo (nome e `over_protocol`
batem). Ou seja: **dentro de `header_body`, se `e->inst->proto_fn.len` é
não-zero, `n` só pode ser essa função** — não precisa olhar `n->is_inline`
para decidir, o corpo tem de ir para o `.h` de qualquer forma.

A correção proposta (confirme você mesmo, não copie sem entender):

```c
static bool header_body(const Emit *e, const KAstNode *n) {
    if (e->inst && e->inst->proto_fn.len) return true;
    return k_em_is_pub_inline(e->ast, n) || (e->inst && n->kind == K_AST_FUNCTION && n->is_inline);
}
```

Confirmei isolando `/tmp/p1/p1.k` (fora do golden, `box` comum, `span` sem
`inline`): sem a correção, o `.h` da instância sai só com o protótipo; é
exatamente o defeito do 072.

## O que NÃO mexer sem oráculo novo

`emit/instance.c` tem **cinco** pontos que também leem `n->is_inline` sobre
nós que `k_em_generic_out` aceita (linhas aproximadas 443, 510, 605, 726, 768
no início desta investigação — confirme as atuais). Todos filtram por
`k_em_generic_out(ast, n)`, que retorna `true` para **qualquer** função sobre
protocolo, mesmo fora de módulo genérico (`emit/generic.c`,
`k_em_generic_out`, primeira linha: `if (over_protocol(ast, n)) return true;`).
Isso sugere que uma função sobre protocolo **declarada dentro de um módulo
genérico** (não é o caso de 065/071/072, que estão em módulos comuns) sem
`inline` pode ter o mesmo defeito, por um caminho diferente (a emissão do
`.c` de uma instância comum, `K_EMIT_C`, não usada pelo caminho de
`emit_protocol_functions` que só chama `K_EMIT_TYPE_H`/`K_EMIT_H`).

**Não tente corrigir isso agora.** Não há golden para esse caso, e o cartão
é só o 072 (função comum, módulo comum). Se, ao investigar, você confirmar
que esses cinco pontos afetam de fato o 072 ou algum golden existente,
corrija com a mesma lógica (tratar como inline quando é função sobre
protocolo) e explique no relatório. Se não afetam nada hoje, **não toque**:
registre a suspeita no relatório para o André decidir se abre um caso novo.

## Aceitação

```sh
make -C tools/cgen -j8 2>&1 | grep -i "warning\|error"   # nada
sh tools/agents/verify.sh                                   # ALL OK
test/run.sh > /tmp/t53.out 2>&1; tail -8 /tmp/t53.out       # 0 fail, 0 wip
```

E, à mão, o 072 byte a byte, os dois perfis:

```sh
for p in c23 c11; do rm -rf /tmp/t53-$p
  tools/cgen/cgen --base-dir base -I golden/cases/072-protocol-function-implicit-inline \
    --profile=$p --stop-after=gen --dest-dir /tmp/t53-$p \
    golden/cases/072-protocol-function-implicit-inline/pf2.k
  diff -r /tmp/t53-$p golden/cases/072-protocol-function-implicit-inline/expected/$p | grep -v "Only in /tmp/t53-$p: keel"
done
```
(sem diferenças, fora os arquivos do prelúdio `keel.h`/`keel.type.h`). Veja
`tools/cgen/cgen --cgen-help` se `--profile` não for o nome certo da flag.

Confirme também, à mão (sem oráculo, não precisa golden):
- O 065 e o 071 continuam byte a byte iguais ao `expected/` deles (a correção
  não pode mudar nada quando `inline` já estava escrito).
- Um probe com `pub inline` explícito sobre a mesma forma do 072 continua
  gerando o mesmo C que sem a palavra (a regra 8 diz que ela é redundante).

## Relatório

`tools/agents/reports/T53-protocol-fn-always-inline.md`, pt-BR: o que mudou
(arquivo a arquivo), a saída final dos comandos de aceitação, a investigação
dos cinco pontos de `instance.c` (afetam ou não, e por quê) e todo ponto
aberto.
