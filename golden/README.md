# golden — casos normativos

Cada caso separa **dois papéis** que não podem se misturar:

    casos/<caso>/<módulo>.k          o fonte keel, e ele é COMPLETO — define os
                                     próprios `priv`, e o que o teste precisa
                                     observar sai por `pub`. O CAMINHO do arquivo
                                     é o do `module` que ele declara: `module
                                     app.cfg;` mora em `app/cfg.k`, senão o caso
                                     viola `module-fora-do-caminho` (spec §4.1)
    casos/<caso>/esperado/c23/       o que o cgen deve produzir, e SÓ isso
    casos/<caso>/esperado/c11/       idem, no outro perfil
    casos/<caso>/prova.c             o arnês; NÃO é saída do transpilador
    casos/<caso>/VERIFICA            quando o que se afirma não é "compila"

**`esperado/`** responde *o cgen produziu o que devia?* — é o alvo de comparação
quando o transpiler existir. Um módulo com `pub` tem **quatro** arquivos, e uma
instância tem os três primeiros (`backend §4.3.2`):

    <módulo>.type.h    L0+L1  os tipos, e as declarações adiantadas dos campos `X *`
    <módulo>.h         L2     a interface: inclui o próprio .type.h, mais protótipos
    <módulo>.impl.h    L3     os corpos `static inline`
    <módulo>.c         L3     os corpos fora de linha

Um módulo que declara `main` tem também a unidade de entrada `main_<módulo>.c`,
porque ela não vai dentro do `.c` do módulo (`backend §5.8`).

**O corte em três não é arrumação, é o que impede ciclo de inclusão.** A
dependência de layout de uma instância corre no sentido contrário do import —
`keel.buffer` importa `keel.outcome`, e o layout de `keel_outcome_keel_buffer_i32`
depende do de `keel_buffer_i32`. Com tipos e assinaturas no mesmo arquivo, os dois
sentidos se encontram e o ciclo é inevitável; separados, cada grafo é acíclico por
uma razão própria (`backend §4.3.1`).

**`prova.c`** responde *o que ele produziu se comporta como a spec diz?* Ele
inclui o gerado, toca **só a interface pública**, e afirma. Qual camada ele
inclui sai da regra 5 do `backend §4.3.2` e é sempre a mesma pergunta: **quem
chama inclui o `.impl.h`**; quem só nomeia um tipo inclui o `.type.h`. Compila junto
com o gerado e roda. Nunca é comparado com nada — é código de teste, para
sempre. Casos cujo `.k` declara `main` não têm `prova.c`: as asserções vivem no
fonte keel e o ponto de entrada é o wrapper gerado.

A separação não é organização: **misturá-los foi o que invalidou a primeira
versão destes casos.** Com o arnês dentro do `esperado.c`, dezesseis casos
ficaram sem `.h` e catorze puseram `int main` no `.c` do módulo — um transpiler
correto falharia a comparação em todos. O runner agora recusa as duas coisas.

## O que o runner checa antes de compilar

O corte em camadas é verificável sem compilador, e um gerado que o violasse
compilaria assim mesmo — a suíte deixaria de ser oráculo justamente da regra que
mata os ciclos. Então `run.sh` começa por três asserções estruturais:

1. todo `.h` de módulo ou instância tem `.type.h` e `.impl.h` ao lado;
2. **um `.type.h` inclui apenas `.type.h`** — é o que faz o grafo de layout ser
   um DAG;
3. **um `.h` não inclui o `.h` de outro módulo** — protótipo não precisa de
   protótipo, e é essa ausência de aresta que deixa a interface de um módulo sem
   arrastar a implementação dos outros.

Falha estrutural sai como `ESTRUT` e conta como falha.

## Para o editor achar os headers

Os headers fixos vivem em `c23/keel/` e `c11/keel/`; os gerados de cada caso,
em `casos/<caso>/esperado/<perfil>/`. Sem saber disso, o clangd não resolve um
`#include "keel/prelude.h"` sequer.

    ./gerar-ccjson.sh

emite `compile_commands.json` com o include path e o `-std` certos por arquivo —
o perfil vem do caminho. Zed, VSCode e vim leem daí. Não é versionado, porque
tem caminhos absolutos: rodar de novo depois de acrescentar um caso.

O arquivo fica em `golden/`, e é onde o clangd o acha: ele sobe do diretório do
arquivo aberto até encontrar. O `.clangd` na raiz do repositório aponta para cá,
para o caso de a raiz do projeto no editor confundir essa busca.

> **`.k` não é assunto do clangd.** Ele atende C, C++ e ObjC, e mais nada. Num
> arquivo keel o que existe é o realce do `editors/zed/` — sem resolução de
> `#include`, sem ir-para-definição, sem diagnóstico. Isso é da alçada do cgen,
> quando ele existir.

## Como roda

    ./run.sh            # gcc; CC=clang ./run.sh para outro compilador

Perfil C23 compila com `-std=c2x` sobre `c23/`; perfil C11 com `-std=c11` sobre
`c11/`. Um caso sem `esperado.c11.c` usa o mesmo arquivo nos dois.

## Os dois perfis são árvores separadas, e isso é normativo

`cgen` emite **um** perfil, já resolvido — o gerado nunca tem `#if` de versão.
Confirmado na prática: o GCC 13 reporta `__STDC_VERSION__ == 202000L` sob
`-std=c2x`, não `202311L`, então detectar C23 pelo pré-processador não funciona.
É a razão de `cgen-tool-spec.md §4.9` ler o perfil da linha de comando.

Os headers fixos (`keel/prelude.h`) e os de módulo e instância da base vivem em
`c23/keel/` e `c11/keel/`.

**Cada árvore é escrita como saída esperada, e nenhuma é derivada da outra.**
Houve um `derivar-c11.sh` que gerava `c11/` de `c23/` por `sed`, e ele saiu: o
que um perfil emite é **escolha do backend**, não uma reescrita textual fixa do
outro. O §9.1 lista as diferenças que existem hoje, e a lista ser curta é um
fato sobre a versão de agora, não um contrato — assim que uma delas deixar de
ser textual, o derivador mentiria em silêncio.

Ele já mentia: `c11/keel/prelude.h` precisa de `<stdbool.h>` e `<assert.h>`, que
o C23 não pede, e `casos/008-constexpr-bloco/esperado/c11/app/cx.c` escreve
`constexpr` de escopo de bloco como macro com nome reescrito (`backend §9.2`) —
nenhum dos dois sai de `sed`. Eram exceções mantidas à mão dentro de um script
que se apresentava como completo, e derivar por cima delas as apagava.

## `prova.c` é um arquivo para os dois perfis

O arnês não é derivado: é o mesmo arquivo compilado sob `-std=c11` e `-std=c2x`.
Então ele **não pode usar grafia que o `backend §9.1` troca** — escreve
`_Alignof` e `_Static_assert`, nunca `alignof` e `static_assert`. Quem ganha a
forma certa por perfil é o gerado, não o teste.

## Casos com VERIFICA

Nem tudo que a spec afirma é "este C compila". O mapeamento de linha do
`backend §6`, por exemplo, só se afirma **fazendo o compilador C falhar** e
conferindo que a mensagem aponta o `.k` com os nomes que o usuário escreveu —
que é o princípio 3.

Um caso com um `VERIFICA` executável não é compilado pelo runner: o script
recebe `$1` compilador, `$2` `-std=…`, `$3` diretório do perfil, e decide.

## Casos com PROBLEMA

Um caso que contém um arquivo `PROBLEMA` é **xfail**: espera-se que o C esperado
*não* compile, e o arquivo registra a lacuna da spec e a saída candidata. Se um
xfail passar a compilar, o runner reporta `XPASS` — o problema foi resolvido e o
arquivo deve sair.
