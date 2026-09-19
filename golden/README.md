# golden — casos normativos

Cada caso separa **dois papéis** que não podem se misturar:

    cases/<caso>/<módulo>.k          o fonte keel, e ele é COMPLETO — define os
                                     próprios `priv`, e o que o teste precisa
                                     observar sai por `pub`. O CAMINHO do arquivo
                                     é o do `module` que ele declara: `module
                                     app.cfg;` mora em `app/cfg.k`, senão o caso
                                     viola `module-path-mismatch` (spec §4.1)
    cases/<caso>/expected/c23/       o que o cgen deve produzir, e SÓ isso
    cases/<caso>/expected/c11/       idem, no outro perfil
    cases/<caso>/proof.c             o arnês; NÃO é saída do transpilador
    cases/<caso>/VERIFY            quando o que se afirma não é "compila"
    cases/<caso>/NOTES                o que o caso encontrou, e a explicação de cada
                                     esperado; o esperado em si abre só com o
                                     cabeçalho de uma linha que o cgen gera

**`expected/`** responde *o cgen produziu o que devia?* — é o alvo de comparação
quando o transpiler existir. O módulo do caso é o que a invocação compila, então
tem **três** arquivos, sempre; uma instância, ou um módulo só importado, tem os
dois primeiros (`backend §4.1`, `§4.3.2`):

    <módulo>.type.h    L0+L1  os tipos, os `constexpr` de módulo, e as declarações
                              adiantadas dos campos `X *`
    <módulo>.h         L2+L3  os .type.h, os protótipos e `extern`, os .h de quem os
                              corpos chamam, e os corpos `static inline` — nessa
                              ordem; é o único include de uso
    <módulo>.c         L3     os corpos fora de linha; inclui o próprio .h

Um módulo genérico não tem `.c`: ele não é unidade compilada (`generic-source-without-instance`),
e o corpo fora de linha de uma instância vai para o `.c` de quem declara
`instance` (`backend §4.4`).

Um módulo que declara `main` tem também a unidade de entrada `main_<módulo>.c`,
porque ela não vai dentro do `.c` do módulo (`backend §5.8`).

**O corte em dois não é arrumação, é o que impede ciclo de inclusão.** A
dependência de layout de uma instância corre no sentido contrário do import —
`keel.buffer` importa `keel.outcome`, e o layout de `keel_outcome_keel_buffer_i32`
depende do de `keel_buffer_i32`. Com tipos e corpos no mesmo arquivo, os dois
sentidos se encontram e o ciclo depende da ordem de entrada; separados, o de
layout é acíclico, e o de chamada fica inofensivo pela ordem das seções do `.h`
(`backend §4.3.1`, `§4.3.2`; o porquê, no rationale, "Dois headers, tipo e uso").

**`proof.c`** responde *o que ele produziu se comporta como a spec diz?* Ele
inclui o gerado, toca **só a interface pública**, e afirma. Ele inclui o `.h` de
cada módulo que usa, como qualquer `.c` de usuário (regra 3 do `backend §4.3.2`). Compila junto
com o gerado e roda. Nunca é comparado com nada — é código de teste, para
sempre. Casos cujo `.k` declara `main` não têm `proof.c`: as asserções vivem no
fonte keel e o ponto de entrada é o wrapper gerado.

A separação não é organização: **misturá-los foi o que invalidou a primeira
versão destes casos.** Com o arnês dentro do `expected.c`, dezesseis casos
ficaram sem `.h` e catorze puseram `int main` no `.c` do módulo — um transpiler
correto falharia a comparação em todos. O runner agora recusa as duas coisas.

## O que o runner checa antes de compilar

O corte em camadas é verificável sem compilador, e um gerado que o violasse
compilaria assim mesmo — a suíte deixaria de ser oráculo justamente da regra que
mata os ciclos. Então `run.sh` começa por asserções estruturais:

1. todo `.h` de módulo ou instância tem `.type.h` ao lado, e não existe `.proto.h`;
2. o módulo de cada caso tem `.h` e `.c` com o nome do símbolo, sob o diretório
   dos componentes-pai (`backend §4.1`);
3. **um `.type.h` inclui apenas `.type.h`** — é o que faz o grafo de layout ser
   um DAG;
4. **num `.h`, todo `#include` de `.type.h` vem antes do primeiro `#include` de
   `.h`** — os protótipos ficam entre os dois blocos, e é essa ordem que faz
   todo protótipo alcançável chegar antes do primeiro corpo.

Falha estrutural sai como `STRUCT` e conta como falha.

## Para o editor achar os headers

Os headers fixos vivem em `c23/keel/` e `c11/keel/`; os gerados de cada caso,
em `cases/<caso>/expected/<perfil>/`. Sem saber disso, o clangd não resolve um
`#include "keel.type.h"` sequer.

    ./gen-ccjson.sh

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

A explicação dos headers da base mora em `NOTA`, pela mesma razão. Os headers do prelúdio (`keel.type.h` e `keel.h` — gerados de
`keel.k`, mas determinísticos: mesmo conteúdo sempre, para um dado perfil) vivem
na raiz de `c23/` e `c11/`, porque `module keel;` não tem componente-pai
(`backend §4.1`); os de módulo e instância da base, em `c23/keel/` e
`c11/keel/`.

**Cada árvore é escrita como saída esperada, e nenhuma é derivada da outra.**
Houve um `derivar-c11.sh` que gerava `c11/` de `c23/` por `sed`, e ele saiu: o
que um perfil emite é **escolha do backend**, não uma reescrita textual fixa do
outro. O §9.1 lista as diferenças que existem hoje, e a lista ser curta é um
fato sobre a versão de agora, não um contrato — assim que uma delas deixar de
ser textual, o derivador mentiria em silêncio.

Ele já mentia: `c11/keel.type.h` precisa de `<stdbool.h>` e `<assert.h>`, que
o C23 não pede, e `cases/008-block-constexpr/expected/c11/app/cx.c` escreve
`constexpr` de escopo de bloco como macro com nome reescrito (`backend §9.2`) —
nenhum dos dois sai de `sed`. Eram exceções mantidas à mão dentro de um script
que se apresentava como completo, e derivar por cima delas as apagava.

## `proof.c` é um arquivo para os dois perfis

O arnês não é derivado: é o mesmo arquivo compilado sob `-std=c11` e `-std=c2x`.
Então ele **não pode usar grafia que o `backend §9.1` troca** — escreve
`_Alignof` e `_Static_assert`, nunca `alignof` e `static_assert`. Quem ganha a
forma certa por perfil é o gerado, não o teste.

## Casos com VERIFY

Nem tudo que a spec afirma é "este C compila". O mapeamento de linha do
`backend §6`, por exemplo, só se afirma **fazendo o compilador C falhar** e
conferindo que a mensagem aponta o `.k` com os nomes que o usuário escreveu —
que é o princípio 3.

Um caso com um `VERIFY` executável não é compilado pelo runner: o script
recebe `$1` compilador, `$2` `-std=…`, `$3` diretório do perfil, e decide.

## Casos com XFAIL

Um caso que contém um arquivo `XFAIL` é **xfail**: espera-se que o C esperado
*não* compile, e o arquivo registra a lacuna da spec e a saída candidata. Se um
xfail passar a compilar, o runner reporta `XPASS` — o problema foi resolvido e o
arquivo deve sair.
