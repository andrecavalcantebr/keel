# T4 — prefixo `isl_` nos 103 nomes que a T3 tornou externos

## Objetivo

A T3 partiu o `parser_islands.c` e tirou o `static` de 103 funções, que agora
têm ligação externa com nomes genéricos (`tok`, `put`, `emit`, `walk`,
`generic`, `keep`…). O M3 vai ter um `emit` próprio, e qualquer outro arquivo
do motor que declare uma função externa com um desses nomes quebra o link. A
tarefa dá a essas 103 funções o prefixo `isl_`, e muda só isso.

## Snapshot (antes de qualquer edição)

```sh
sh tools/agents/verify.sh snapshot /tmp/keel-snap-T4
```

## Quais nomes

São as funções de ligação externa definidas em `parser_islands*.c` cujo nome
não começa por `k_`. Depois de um build, a lista exata é:

```sh
nm tools/cgen/obj/engine/parser_islands*.o | awk '$2=="T"{print $3}' | grep -v '^k_' | sort
# 103 nomes hoje
```

Elas são as mesmas cujos protótipos estão em
`tools/cgen/src/engine/parser/parser_islands_internal.h`. `k_collect_islands` e
qualquer outro `k_*` ficam como estão.

## Como renomear

Renomeie **só as referências à função**: a definição, o protótipo no header e
cada chamada ou uso como ponteiro de função. Vários desses nomes também são
nomes de variável local, parâmetro ou campo de struct no mesmo código
(`tok`, `walk`, `binder`, `generic`, `kt`, `emit`, `keep`…). Esses **não**
mudam. Um `sed 's/\bwalk\b/isl_walk/g'` cego erraria, porque renomearia campos
e variáveis. Seja qual for o método (por nome seguido de `(`, ou com o
compilador como guia), as aceitações abaixo dizem se acertou.

Escopo de arquivos: `tools/cgen/src/engine/parser_islands*.c`,
`parser_islands_internal.h` e `tools/agents/reports/T4-islands-prefix.md`.
Nenhum outro arquivo chama essas funções. Se o build mostrar que algum chama,
pare e registre.

Não realinhe nada. Uma continuação de assinatura que ficar desalinhada em 4
colunas fica assim; o relatório lista quantas ficaram.

## Aceitação

As quatro têm de passar:

```sh
# 1. só nomes k_ e isl_ saem dos objetos da passagem
make -C tools/cgen -j8 >/dev/null
nm tools/cgen/obj/engine/parser_islands*.o | awk '$2=="T"{print $3}' | grep -v -E '^(k_|isl_)'   # nada
nm tools/cgen/obj/engine/parser_islands*.o | awk '$2=="T"{print $3}' | grep -c '^isl_'           # 103

# 2. o prefixo é a única mudança: tirá-lo reproduz o HEAD byte a byte
#    (hoje não existe nenhum "isl_" em tools/cgen/src)
for f in tools/cgen/src/engine/parser_islands*.c tools/cgen/src/engine/parser/parser_islands_internal.h; do
    sed 's/\bisl_//g' "$f" | cmp -s - <(git show HEAD:"$f") || echo "DIFF $f"
done                                                                                             # nada
#    (rode com bash, por causa do <( ))

# 3. nenhum nome de variável, parâmetro ou campo foi renomeado: todo isl_ é
#    seguido de '(' ou é o nome declarado num protótipo/definição
grep -n -E '\bisl_[a-z_0-9]+\b' tools/cgen/src/engine/parser_islands*.[ch] \
    | grep -v -E '\bisl_[a-z_0-9]+\(' | head                                                      # nada, ou só usos como ponteiro de função, cada um justificado no relatório

# 4. comportamento igual
sh tools/agents/verify.sh compare /tmp/keel-snap-T4                                               # ALL OK
```

A aceitação 2 garante que nada além do prefixo mudou. A 3 garante que o
prefixo não caiu em nome que não é de função. Se a 3 acusar um uso legítimo
(um `&isl_x` ou um `isl_x` passado como argumento), explique cada um no
relatório.

## Relatório

`tools/agents/reports/T4-islands-prefix.md`, em pt-BR: o método usado, a
contagem por arquivo, os casos de nome que também é variável ou campo (e
como os separou), a saída das quatro aceitações e as dúvidas.
