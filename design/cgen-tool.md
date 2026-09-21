# cgen — desenho da implementação

> © 2026 Faculdade de Engenharia Elétrica e de Computação, Universidade
> Federal do Amazonas (FEEC/UFAM). Licenciado sob
> [CC BY-SA 4.0](../LICENSE-DOCS.md) ([tradução](../LICENSE-DOCS.pt.md)) —
> plano de implementação, não contrato normativo; o `cgen` e a Base keel têm
> licença própria, ver [`LICENSE.md`](../LICENSE.md). Escrita e revisão
> tiveram auxílio de Claude Opus (Anthropic), sob direção humana.

**Documento de implementação.** O contrato da ferramenta é
[`cgen-tool-spec.md`](../cgen-tool-spec.md) (a "spec da ferramenta"); o do
conteúdo gerado, [`keel-c-backend.md`](../keel-c-backend.md) (o "backend").
Este documento é o de cima: ele reparte o cgen, e cada peça tem o seu —
[`lexer-design.md`](lexer-design.md), [`parser-design.md`](parser-design.md),
[`codegen-design.md`](codegen-design.md) e
[`diag-design.md`](diag-design.md). Este documento não acrescenta
regra de linguagem nem de conteúdo: ele fixa **o que a spec da ferramenta deixa
à implementação** — algoritmo de partição da linha de comando, formato das
fases de depuração, montagem da chamada ao `cc`, organização do código — e dá
entrada e saída esperadas para casos simples, para que a implementação possa
ser escrita e testada sem reler os normativos a cada passo.

Onde este texto **decide** algo que a spec deixa aberto, a decisão vem marcada
**[D*n*]** e listada na §12. Onde encontra os normativos em contradição entre
si, a pendência vem marcada **[P*n*]** e listada na §13 — nesses pontos a
implementação segue o que está dito aqui até o normativo ser corrigido.

---

## 1. Escopo

O `cgen` é um driver no molde do gcc (spec da ferramenta §4): lê **um** fonte —
um `.k`, ou uma instância pedida por `--instance` —, carrega recursivamente os
`import`, gera sob `--dest-dir` os headers de tudo que alcança e o `.c` só do que
compila, e chama o compilador C sobre esse `.c`. Sem fonte na linha, é
transparente: repassa tudo ao `cc`.

Fora do escopo desta versão, pela spec da ferramenta §9: arquivo de interface
por módulo (a v1 reparseia, §5 da spec), manifesto, arquivo de configuração,
modo de programa inteiro.

---

## 2. Linha de comando

### 2.1 Sinopse

```plain
cgen [opções do cgen] [opções do cc] [arquivo.k] [outros arquivos]
cgen [opções do cgen] [opções do cc] --instance "<M.mod> <args>" [outros arquivos]
```

A ordem entre opções do cgen e do cc é livre; a ordem **entre as do cc** é
preservada no repasse.

### 2.2 Opções do cgen

Conjunto fechado, o da spec da ferramenta §4.1. Toda opção desta tabela é
**consumida e nunca repassada**.

| Opção | Formas aceitas | Valor | Padrão |
| --- | --- | --- | --- |
| `-I` | `-I dir`, `-Idir` | raiz de busca de módulo; repetível; **também repassada** (§2.3) | `.`, só quando nenhum `-I` é dado |
| `--dest-dir` | `--dest-dir dir`, `--dest-dir=dir` | raiz dos gerados | `./gen` |
| `--base-dir` | `--base-dir dir`, `--base-dir=dir` | raiz da base (§2.7) | `<executável>/../lib/base` |
| `--instance` | `--instance "U"`, `--instance="U"` | uso qualificado por inteiro, no lugar do `.k` (§2.8) | — |
| `--stop-after` | `--stop-after=F`, `--stop-after F` | `lex` \| `parse` \| `gen` | não para |
| `--checks` | `--checks=V`, `--checks V` | `on` \| `off` | `on` |
| `--line` | `--line=V`, `--line V` | `on` \| `off` | `on` |
| `--main` | `--main M`, `--main=M` | nome de módulo (`app.cfg`, não caminho) | não gera |
| `--cc` | `--cc=P`, `--cc P` | programa do compilador C | `cc` |
| `--pedantic-names` | — | teto de nome 63 | desligado |
| `--parallel-lowering` | `=V` ou ` V` | `auto` \| `serie` \| `openmp` | `auto` |
| `--profile` | `=V` ou ` V` | `auto` \| `c11` \| `c23` | `auto` |
| `-f` | — | força regeração | desligado |
| `--cgen-version` | — | imprime `cgen <versão>` em stdout, sai 0 | — |
| `--cgen-help` | — | imprime a sinopse e esta tabela, sai 0 | — |

As duas formas (`=` e argumento separado) valem para toda opção longa com valor
**[D2]**. Valor fora do enumerado é `invalid-option` (código 2). Opção que
exige valor e está no fim da linha também é `invalid-option`.

Repetição: a última ocorrência vence, salvo `-I`, que acumula. `--instance`
repetido é `multiple-sources`.

`--version` e `--help`, sem o prefixo, **não são do cgen**: vão para o `cc`.
`cgen --version` imprime a versão do gcc — é o que um build que sonda `$(CC)
--version` espera.

### 2.3 Opções do cc que o cgen também lê

Repassadas íntegras, na posição em que apareceram, e lidas (spec §4.1):

| Opção | O cgen lê para |
| --- | --- |
| `-I dir` / `-Idir` | raízes de módulo (§2.2) |
| `-std=X` | `--profile=auto`: `c23`, `gnu23`, `c2x`, `gnu2x` → C23; qualquer outro, ou ausência → C11 |
| `-fopenmp`, `-fopenmp=…` | `--parallel-lowering=auto` |
| `-MD`, `-MMD`, `-MF f`, `-MT t`, `-MQ t` | depfile (§7) |
| `-c`, `-S`, `-E`, `-o f` | modo (§2.6) e nome do depfile |
| `-W<nome>`, `-Wno-<nome>` | **consumida** se `<nome>` está no catálogo da linguagem; repassada senão (spec §7) |
| `-Werror` | lida **e** repassada |

O catálogo de nomes de diagnóstico (keel-spec §6.2) é compilado no binário
como tabela ordenada; o filtro de `-W` é busca binária nela.

### 2.4 Opções do cc que levam argumento separado

Para achar o `.k` e não confundir argumento de opção com arquivo, o cgen
precisa saber quais opções do cc **consomem a palavra seguinte**. A lista é
fechada e vem do gcc:

```plain
-o  -MF  -MT  -MQ  -I  -D  -U  -L  -l  -x  -include  -imacros  -isystem
-iquote  -idirafter  -iprefix  -iwithprefix  -isysroot  -Xlinker
-Xassembler  -Xpreprocessor  -u  -T  -z  --param  -aux-info
```

Cada uma dessas, quando escrita **sem** valor colado (`-o x`, não `-ox`),
leva a palavra seguinte junto no repasse, e a palavra seguinte não é
examinada como arquivo. Opção do cc fora dessa lista é tratada como palavra
única. Um `.k` só é reconhecido em posição de operando.

### 2.5 Algoritmo de partição

```plain
para cada palavra w de argv[1..], em ordem:
    se w é opção do cgen (§2.2):          consome (e o valor, se separado)
    senão se w é -W<nome> do catálogo:    consome
    senão se w está na lista da §2.4 e não tem valor colado:
                                          repassa w e a palavra seguinte
    senão se w começa com '-':            repassa w
    senão se w termina em ".k":           registra como fonte; guarda a posição
    senão:                                repassa w (arquivo .c/.o/.a …)
depois:
    -I (do cgen e do cc) → lista de raízes, em ordem; nenhuma → "."
    fontes = .k registrados + --instance
    mais de um → multiple-sources (código 2)
```

Uma palavra que é `-I` vale para os dois lados: entra nas raízes **e** no
repasse, na mesma posição. O `.` padrão só existe quando nenhum `-I` é dado:
se fosse sempre raiz, `cgen -I src src/a.k` seria sempre
`source-in-multiple-roots`, porque `src/a.k` está sob `.` e sob `src`.

### 2.6 Modos

| Linha | O que o cgen faz |
| --- | --- |
| só `--cgen-version`/`--cgen-help` | imprime e sai 0 |
| sem fonte | `execvp` do `cc` com o repasse, sem acrescentar nada; o código de saída é o do `cc` |
| fonte + `--stop-after=lex` | lexa **só** o `.k` dado, imprime (§5.1); não carrega import. Com `--instance`, lexa o `.k` do genérico |
| fonte + `--stop-after=parse` | carrega o fecho de import, imprime o módulo dado (§5.2); não escreve nada |
| fonte + `--stop-after=gen` | gera; escreve depfile se `-MD`/`-MMD`; não chama `cc` |
| fonte com `-c`, `-S` ou `-E` | gera; chama o `cc` com o `.c` da invocação no lugar do fonte |
| fonte sem `-c`/`-S`/`-E` | idem, e o `cc` compila e linka; com `--main`, a unidade de entrada entra na mesma chamada **[D3]** |

Em modo `--stop-after`, as opções repassadas são **lidas** (perfil, lowering,
depfile) e o resto é ignorado — `-o` inclusive.

Com `--main` e `-c`, a unidade de entrada é **gerada mas não compilada**: `-c`
com `-o` e dois `.c` é erro do gcc, e a unidade tem regra própria no build
(`cc -c gen/main_app.c`) **[D3]**.

Um `.k` de módulo genérico sem `--instance` é `generic-source-without-instance` (código 2),
depois do parse da linha `module`, antes de qualquer escrita.

### 2.7 A base

A base é o diretório com `keel.k` e `keel/*.k` (spec §8). Resolução:

1. `--base-dir`, se dado.
2. `<dir do executável real>/../lib/base`. O executável é resolvido até o
   arquivo real, seguindo symlink:

   | Sistema | Chamada |
   | --- | --- |
   | Linux | `readlink("/proc/self/exe")` — o kernel devolve o alvo, não o link |
   | macOS | `_NSGetExecutablePath` + `realpath` |
   | FreeBSD | `sysctl` `KERN_PROC_PATHNAME` |
   | Windows | `GetModuleFileNameW` |
   | outro | `argv[0]`: com `/`, `realpath`; sem `/`, busca no `PATH` e `realpath` |

Nenhuma das duas contendo `keel.k`: `base-not-found`. Sem variável de
ambiente.

A base entra como **última** raiz de módulo, depois de todas as `-I`. **Ela não
vai para o `cc`**: só tem `.k`. Os headers dela são gerados em `--dest-dir`
como os de qualquer módulo importado.

No repositório, durante o desenvolvimento, a base é `/base` (a raiz do repo),
passada por `--base-dir`; o Makefile de `tools/cgen` faz isso nos testes.

### 2.8 `--instance`

O argumento é o uso como a declaração `instance` o escreve, **inteiramente
qualificado** (spec §4.3): `"keel.buffer.buffer i32"`, `"coll.stack
app.geom.Point"`, `"blocos.bloco(3) f32"`. Algoritmo:

1. Lexa o argumento com o lexer do `.k` — é texto keel.
2. O primeiro nome qualificado se parte no último `.`: à esquerda, o módulo; à
   direita, o modificador.
3. `carrega` o módulo pelas raízes, como um `import`. Não achado:
   `module-not-found`. Achado e não genérico, ou sem esse modificador:
   `invalid-instance`.
4. Os argumentos são resolvidos como num uso, carregando cada módulo que
   qualificam. Aridade ou espécie que não casa com `dim`/`tags`/`type` da linha
   `module`: `invalid-instance`.
5. A invocação compila a instância: os dois headers dela e o `.c` com o símbolo
   dela (`gen/keel/keel_buffer_i32.c`). Genérico todo `pub inline`:
   `redundant-instance` (`warning`), e o `.c` sai só com o include.

A identidade da invocação é o símbolo da instância — é ele que nomeia o `.c`, o
`.o` derivado e o alvo do depfile.

---

## 3. Arquitetura do código

### 3.1 Arquivos

A spec da ferramenta §3 já separa E/S de tradução, e diz que a separação "é de
responsabilidade, não de empacotamento". **[D8] O fonte torna essa fronteira
visível: dois diretórios, um executável.**

```plain
tools/cgen/src/
    tool/       depende da invocação e do sistema de arquivos
    engine/     depende só do fonte
```

| `tool/` | Responsabilidade | Não faz |
| --- | --- | --- |
| `main.c` | `argv` → `KInvocation`; escolhe o modo; único `exit` do programa | não traduz |
| `args.c` | partição (§2.5), validação de valores | não resolve caminho |
| `paths.c` | normalização, raiz de um fonte, módulo ↔ caminho `.k`, símbolo ↔ gerado, executável real (§2.7) | não abre arquivo |
| `tool.c` | `carrega`/`processa` (spec §3), pilha de carga, memoização, fecho de mtime | não parseia |
| `writer.c` | spec da ferramenta §6: comparar, temporário, `rename` | não gera conteúdo |
| `depfile.c` | §7 | — |
| `cc.c` | monta o `argv` do `cc`, `posix_spawnp` + `waitpid` | — |
| `report.c` | escreve o diagnóstico em `stderr`, na ordem do §4 | não decide severidade |

| `engine/` | Responsabilidade | Não faz |
| --- | --- | --- |
| `lexer.c` | [`lexer-design.md`](lexer-design.md) | — |
| `parser.c` | ilhas, declarações, resolução (linguagem §4.4) | — |
| `symtab.c` | tabela de símbolos, fecho de imports, instâncias pedidas | — |
| `emit/` | [`codegen-design.md`](codegen-design.md) — os três artefatos | — |
| `diag.c` | `KDiagnosticSink`: acumula, classifica, filtra por `-W`, conta `error` | não formata para terminal, não escreve |

> **Nada em `engine/` abre arquivo, escreve arquivo ou termina o processo.**

A fronteira é verificável sem executar nada, e **o build a verifica**: o alvo
`boundary` do `tools/cgen/Makefile` recusa qualquer fonte de `engine/` que
inclua `<stdio.h>`, `<stdlib.h>`, `<unistd.h>`, `<fcntl.h>` ou `<sys/*.h>`, e
roda junto com `all`. É a mesma espécie de invariante estrutural que o runner
do golden aplica ao gerado (I1 e I2), e pela mesma razão — uma regra que só se
verifica lendo o código é uma regra que vai se perder.

O que a separação compra, além da disciplina: o motor é testável **sem sistema
de arquivos**. Um teste entrega bytes e um `KLoader` de mentira que devolve
módulos de um vetor em memória, e confere os buffers de saída. Nenhum
diretório temporário, nenhum `mkstemp`, nenhuma limpeza.

`diag.c` fica no motor e `report.c` na ferramenta porque as duas metades do
diagnóstico têm donos diferentes: **qual** é a condição e **qual** a severidade
dependem só do fonte; a cor no terminal, a ordem entre módulos e o destino
dependem da invocação.

O parser chama de volta `tool.c` por ponteiro de função — é a única aresta que
sobe do motor para a ferramenta, e ela é de dados, não de controle de processo:

```c
typedef enum { K_LOAD_OK, K_LOAD_ALREADY, K_LOAD_NOT_FOUND, K_LOAD_ERROR } KLoadResult;

typedef struct KModule KModule;            /* public symbols, imports, closure mtime */

typedef struct {
    KLoadResult (*load)(void *tool, keel_slice_char module_name, KModule **out);
    void *tool;
} KLoader;

/* process(source, name) → artifacts, diagnostics */
bool k_process(keel_slice_char source, keel_slice_char module_name, bool compiled,
               KLoader *loader, KDiagnosticSink *diag, KArtifacts *out);
```

`compiled` diz se o módulo é o da invocação: só então `KArtifacts` recebe o
`.c`. `K_LOAD_ALREADY` inclui o caso "em carga": `tool.c` sabe que o módulo está
na pilha e devolve o erro de ciclo com a cadeia; o parser emite
`circular-import` na posição do `import`.

### 3.2 Os tipos de apoio

O `cgen` é escrito em C sobre um conjunto de tipos de apoio — `keel_arena`,
`keel_slice_char`, `keel_buffer_char`, `keel_outcome_*` —, na mesma forma que o
backend gera, para que o código do cgen já use as técnicas de keel. Eles vêm do
`transform` (`tools/transform`), que **não tem relação com o cgen**: é uma
ferramenta de substituição textual (`$T`, `$N`, `$E`) sobre moldes próprios, que
não são keel e não são a `/base`. Ela gera em `tools/cgen/gen/` só as instâncias
de que o cgen precisa, só em headers. Uma instância nova (por exemplo `buffer
KToken`) entra acrescentando o tipo à lista de `tools/transform/base/Makefile`,
com `-a` apontando o header do tipo.

A memória do `cgen` inteiro vem de uma `keel_arena` por invocação; nada é
liberado antes do fim do processo.

### 3.3 Fluxo de uma invocação

1. `args` particiona; erros de invocação saem com código 2 antes de qualquer E/S.
2. Sem fonte: `execvp` do `cc` (modo transparente).
3. Resolve a base (§2.7).
4. **Com `.k`:** normaliza o caminho (remove `./`, colapsa `//`, resolve `..`
   lexicalmente); acha a **única** raiz que o contém (`source-outside-roots`,
   `source-in-multiple-roots`). O nome esperado do módulo é o caminho relativo à
   raiz, sem `.k`, com `/` → `.`. **Com `--instance`:** §2.8.
5. `processa` o fonte com `compiled = true`. O `module` declarado tem de bater
   com o nome esperado (`module-path-mismatch`, da linguagem, código 1).
   Módulo genérico sem `--instance`: `generic-source-without-instance`.
6. O módulo `keel` é carregado implicitamente antes do primeiro `import`. Cada
   `import` → `carrega`: procura `<raiz>/<a>/<b>.k` em cada raiz, na ordem, base
   por último; o primeiro achado vence; `processa` com `compiled = false`.
7. Na volta da recursão, cada módulo sabe o `mtime` máximo do seu fecho, e
   decide gerar ou não pela spec §5 (o mais antigo dos seus gerados contra esse
   máximo; `-f` força). Instância: spec §5.1.
8. Com algum `error` no diagnóstico: nada é escrito, sai 1.
9. `writer` grava tudo que foi gerado em memória — compara antes; temporário
   `<arquivo>.tmp.<pid>` no mesmo diretório; `rename`.
10. `--main`: gera `main_<símbolo>.c` (§5.3).
11. Depfile, se pedido (§7).
12. Chama o `cc` (§6), salvo `--stop-after`.

---

## 4. Diagnóstico e código de saída

Formato e códigos são da spec da ferramenta §7. Complementos de implementação:

- Diagnósticos vão para `stderr`, em **ordem de fonte** por módulo, e na ordem
  de carga entre módulos. Nada de ordem de hash.
- Linha e coluna começam em 1; coluna conta **bytes** desde o início da linha
  física (o que o gcc faz com `-fdiagnostics-column-unit=byte`).
- O caminho impresso é o normalizado, relativo ao diretório corrente quando o
  fonte está sob ele, absoluto senão.
- Erro de ferramenta sem posição: `cgen: error: <message> [<name>]`.

| Código | Situação |
| --- | --- |
| 0 | sucesso (keel e `cc`) |
| 1 | `error` da linguagem; nada escrito, `cc` não chamado |
| 2 | erro da ferramenta (spec §7.1) |
| *n* | o `cc` saiu com *n* ≠ 0: o `cgen` sai com o mesmo *n* |
| 128+*s* | o `cc` morreu pelo sinal *s* |

---

## 5. Saídas das fases

As saídas de `--stop-after=lex` e `=parse` são **de depuração**: não fazem
parte da superfície congelada da spec §2 (linha de comando, pasta de saída,
formato de diagnóstico, código de saída). Mudam quando for útil; os testes que
as usam mudam junto.

### 5.1 `--stop-after=lex`

Uma token por linha, em `stdout`:

```plain
<arquivo>:<linha>:<coluna>: <classe> "<grafia>"
```

`<classe>` sai dos predicados do [lexer-design §5](lexer-design.md#5-predicados-de-classificação),
calculados sobre a grafia **lógica** **[D7]**, na ordem: `pp-if`, `pp-else`,
`pp-endif`, `pp-other` (pela `TKPpKind`), depois `cword`, `ident`, `number`,
`string`, `char`, `punct`; nenhum casando, `other`. `<grafia>` é a vista
**física** — com emendas —, escapada como literal C (`\\`, `\"`, `\n`, `\r`,
`\t`, `\xHH` para os demais bytes de controle). A última linha é
`<arquivo>:<linha>:<coluna>: eof`. Diagnósticos léxicos vão para `stderr` e o
código de saída segue a §4.

### 5.2 `--stop-after=parse`

Em `stdout`, o módulo pedido, em blocos fixos e nesta ordem. Campos separados
por um TAB; a última coluna é sempre a posição no fonte.

```plain
module	<nome>	<pos>
import	<módulo> [as <alias>] [types]	<pos>
import_c	<cabeçalho como escrito>	<pos>
decl	<pub|priv> [inline] <espécie>	<nome keel>	<símbolo C>	<pos>
inst	<modificador> <argumento>	<símbolo C>	<pos do primeiro uso>
ilha	<espécie>	<detalhe>	<pos>
```

`<espécie>` de `decl`: `func`, `var`, `const`, `constexpr`, `type`,
`modifier`, `tags`. `ilha` lista, em ordem de fonte, toda construção keel
reconhecida dentro de corpo: `defer`, `foreach`, `match`, `builtin
<modificador>.<verbo> → <símbolo>`, `ref`, `array`, `parallel` etc. O C opaco
entre ilhas não aparece.

Os módulos importados são carregados (é preciso, para resolver) e não são
impressos.

### 5.3 `--stop-after=gen` e os arquivos gerados

Escreve os arquivos e não chama o `cc`. Sob `--dest-dir D` (backend §4.1):

| Quem | Arquivos |
| --- | --- |
| módulo compilado `a.b` | `D/a/a_b.type.h`, `D/a/a_b.h`, `D/a/a_b.c` |
| módulo importado `a.b` | `D/a/a_b.type.h`, `D/a/a_b.h` |
| o prelúdio `keel`, sempre importado | `D/keel.type.h`, `D/keel.h` |
| genérico importado `keel.buffer` | `D/keel/keel_buffer.type.h`, `D/keel/keel_buffer.h` — só o que não menciona parâmetro (backend §4.4.1) |
| instância usada `buffer i32` | `D/keel/keel_buffer_i32.type.h`, `D/keel/keel_buffer_i32.h` |
| instância compilada por `--instance` | os dois headers e `D/keel/keel_buffer_i32.c` |
| `--main a.b` | `D/main_a_b.c` |

Todo arquivo gerado começa com uma linha de cabeçalho, sem versão nem data
(spec §6.1):

```c
/* <path relative to D> — generated from <.k path relative to the root> by cgen, C23 profile. */
```

O include guard é o caminho relativo a `D` em maiúsculas, com todo caractere
fora de `[A-Z0-9]` virando `_`: `app/app_cfg.type.h` → `APP_APP_CFG_TYPE_H`.

---

## 6. A chamada ao `cc`

```plain
<--cc> <repasse, na ordem, com o fonte trocado por D/<símbolo>.c> [D/main_<símbolo>.c] -I D
```

- O `.c` gerado ocupa **a posição do fonte** na linha — a do `.k`, ou a do
  `--instance` —; os outros operandos (`.c`, `.o` escritos à mão) ficam onde
  estavam.
- `-I D` vai **no fim**, depois de toda `-I` do usuário, que mantém a
  precedência **[D6]**. A base não entra (§2.7).
- A unidade de entrada, quando há e não há `-c`, vai logo depois do `.c` da
  invocação.
- Depfile: ver §7 — o cgen troca o `-MF` do usuário por um temporário.

O processo é criado com `posix_spawnp`; `stdin`, `stdout` e `stderr` são
herdados, então a saída do `cc` chega ao terminal sem passar pelo cgen.

---

## 7. Depfile

Sem `-MD`/`-MMD`, nada. Com um deles:

1. **Nome final**: `-MF f` se houver; senão `-o` com o sufixo trocado por
   `.d`; senão `<símbolo>.d` no diretório corrente, com `<símbolo>` o nome base
   do `.c` da invocação (o que o gcc faria).
2. **Alvo**: `-MT`/`-MQ` se houver; senão o `-o`; senão `<símbolo>.o`. Em
   `--stop-after=gen`, o alvo é `D/<símbolo>.c`.
3. **Metade do cc**: o cgen repassa `-MD`/`-MMD` e **troca** o `-MF` por
   `-MF <final>.tmp.<pid>`. Depois que o `cc` sai com 0, lê a regra do
   temporário.
4. **Metade do cgen**: o fecho de `.k` alcançável por `import`, em ordem de
   carga, incluindo os `.k` da base; com `--instance`, também o `.k` do
   genérico e os dos argumentos.
5. **Fusão**: uma regra só — alvo, dependências do `cc`, depois os `.k`. Linhas
   quebradas com ` \` a cada 72 colunas, como o gcc. Grava pelo `writer` (§3.3,
   passo 9) e apaga o temporário.

`-MP` é repasse normal; os alvos falsos que ele gera vêm do temporário e são
copiados depois da regra fundida, acrescidos de um alvo falso por `.k`.

---

## 8. Casos de uso

Todos a partir de um diretório vazio, com a instalação em `/opt/keel` (base em
`/opt/keel/lib/base`) e `/opt/keel/bin` no `PATH`.

### 8.1 Versão e ajuda

```sh
$ cgen --cgen-version
cgen 0.1.0
$ echo $?
0
$ cgen --version          # comes from the cc
gcc (GCC) 14.2.1 …
```

### 8.2 Link transparente

```sh
$ cgen main.o geom.o -o prog -lm
```

Sem fonte: o cgen executa `cc main.o geom.o -o prog -lm` e sai com o código
dele. Nada é gerado e `gen/` não é criado.

### 8.3 Olá, com ponto de entrada

`hello.k`:

```keel
module hello;
import_c <stdio.h>;

pub int main(int argc, char **argv) {
    puts("hi");
    return 0;
}
```

```sh
$ cgen -std=c23 -Wall --main hello hello.k -o hello
```

Raiz `.` (nenhum `-I`); módulo esperado `hello`, declarado `hello`. Perfil C23
por `-std=c23`. Arquivos escritos:

```plain
gen/keel.type.h  gen/keel.h            (keel.k, importado)
gen/hello.type.h   gen/hello.h   gen/hello.c (hello.k, compilado)
gen/main_hello.c
```

`gen/hello.type.h` — `import_c` vai na camada mais baixa, com `#line` (backend
§4.1 e §6):

```c
/* hello.type.h — generated from hello.k by cgen, C23 profile. */
#ifndef HELLO_TYPE_H
#define HELLO_TYPE_H
#include "keel.type.h"
#line 2 "hello.k"
#include <stdio.h>
#endif /* HELLO_TYPE_H */
```

`gen/hello.h` — o protótipo é declaração levada a header, e ganha `#line`
com a linha dela; nenhum corpo `inline` depois dele:

```c
/* hello.h — generated from hello.k by cgen, C23 profile. */
#ifndef HELLO_H
#define HELLO_H
#include "hello.type.h"
#line 4 "hello.k"
int hello_main(int argc, char **argv);
#endif /* HELLO_H */
```

`gen/hello.c` — um `#line 1` depois dos includes, e toda linha do fonte tem a
sua: `module` e `import_c` deixam linha vazia (backend §6, regra 3); o corpo é
copiado com as quebras intactas, `pub` sai e o nome recebe o prefixo, na mesma
linha:

```c
/* hello.c — generated from hello.k by cgen, C23 profile. */
#include "hello.h"
#line 1 "hello.k"



int hello_main(int argc, char **argv) {
    puts("hi");
    return 0;
}
```

`gen/main_hello.c` — inclui o `.h` do módulo, como qualquer `.c` (regra 3 do
backend §4.3.2):

```c
/* main_hello.c — entry unit, generated by --main hello. */
#include "hello.h"
int main(int argc, char **argv) { return hello_main(argc, argv); }
```

A chamada ao compilador C:

```sh
cc -std=c23 -Wall gen/hello.c gen/main_hello.c -o hello -I gen
```

Uma segunda execução idêntica: tudo está em dia, o cgen reparseia (para ter
os símbolos), não escreve nada — nenhum `mtime` muda — e chama o `cc` de novo,
porque ligar é o que foi pedido.

### 8.4 `--stop-after=lex`

```sh
$ cgen --stop-after=lex hello.k | head -12
hello.k:1:1: ident "module"
hello.k:1:8: ident "hello"
hello.k:1:11: punct ";"
hello.k:2:1: ident "import_c"
hello.k:2:10: punct "<"
hello.k:2:11: ident "stdio"
hello.k:2:16: punct "."
hello.k:2:17: ident "h"
hello.k:2:18: punct ">"
hello.k:2:19: punct ";"
hello.k:4:1: ident "pub"
hello.k:4:5: cword "int"
```

Emenda e diretiva, de `t.k` com `ret\` + newline + `urn x;` e `#if X` na linha
seguinte:

```plain
t.k:1:1: cword "ret\\\nurn"
t.k:2:5: ident "x"
t.k:2:6: punct ";"
t.k:3:1: pp-if "#if X\n"
t.k:4:1: eof
```

A grafia impressa é a física, com a emenda; a classe é calculada sobre a
grafia lógica, `return`, e por isso sai `cword` **[D7]**.

### 8.5 `--stop-after=parse`

`golden/cases/001-arena-buffer-defer`, a partir da raiz do caso:

```sh
$ cgen --base-dir ../../../base --stop-after=parse app/cfg.k
module	app.cfg	app/cfg.k:1:1
import	keel.arena as arena types	app/cfg.k:3:1
import	keel.buffer as buffer types	app/cfg.k:4:1
import	keel.outcome as outcome types	app/cfg.k:5:1
import_c	<stdio.h>	app/cfg.k:6:1
decl	pub constexpr	MAX	app_cfg_MAX	app/cfg.k:8:1
decl	pub func	soma	app_cfg_soma	app/cfg.k:11:1
decl	pub func	soma_scratch	app_cfg_soma_scratch	app/cfg.k:30:1
inst	outcome i32	keel_outcome_i32	app/cfg.k:11:5
inst	buffer i32	keel_buffer_i32	app/cfg.k:17:5
ilha	ref	fp	app/cfg.k:13:11
ilha	builtin	outcome.fail → keel_outcome_i32_fail	app/cfg.k:14:21
…
```

(A lista de ilhas segue até o fim de `sum_scratch`; o teste compara a saída inteira.)

### 8.6 Compilação separada com depfile

`src/main.k` (`module main;`, importa `geom`), `src/geom.k` (`module geom;`):

```sh
$ cgen -I src -MMD -c src/main.k -o main.o -O2
```

Escreve `gen/main.type.h`, `gen/main.h`, `gen/main.c`; os dois headers
de `geom` se desatualizados — **nunca `gen/geom.c`** —; e os do prelúdio.
Executa:

```sh
cc -I src -MMD -c gen/main.c -o main.o -O2 -MF main.d.tmp.<pid> -I gen
```

e escreve `main.d`:

```make
main.o: gen/main.c gen/main.h gen/main.type.h \
 gen/keel.type.h gen/geom.h gen/geom.type.h \
 src/main.k src/geom.k /opt/keel/lib/base/keel.k
```

`geom.o` tem regra própria (`cgen -I src -MMD -c src/geom.k -o geom.o`), e é
ela que escreve `gen/geom.c`.

### 8.7 Instância pré-compilada

```sh
$ cgen -std=c23 -c --instance "keel.buffer.buffer i32" -o keel_buffer_i32.o
cgen: warning: keel.buffer.buffer is entirely pub inline; the .c will have no body [redundant-instance]
```

Escreve `gen/keel/keel_buffer_i32.type.h`, `gen/keel/keel_buffer_i32.h`,
`gen/keel/keel_buffer_i32.c` (só `#include "keel/keel_buffer_i32.h"`), os
headers de `keel.buffer`, `keel.slice`, `keel.outcome`, `keel.arena` e do
prelúdio. Executa `cc -std=c23 -c gen/keel/keel_buffer_i32.c -o
keel_buffer_i32.o -I gen`.

### 8.8 Erros

```sh
$ cgen a.k b.k
cgen: error: more than one source in the invocation [multiple-sources]
$ echo $?
2

$ cgen --profile=c99 hello.k
cgen: error: invalid value 'c99' for --profile; expected auto, c11 or c23 [invalid-option]
$ echo $?
2

$ cgen -I . -I src src/a.k
cgen: error: src/a.k is under more than one root: '.' and 'src' [source-in-multiple-roots]

$ cgen --base-dir base base/keel/buffer.k
cgen: error: keel.buffer is a generic module; use --instance [generic-source-without-instance]

$ cgen --instance "keel.buffer i32"
cgen: error: 'keel.buffer' does not name a modifier in full; module 'keel' is not generic [invalid-instance]

$ cat src/a.k
module b;
$ cgen -I src src/a.k
src/a.k:1:8: error: the module declares 'b', but the path under root 'src' says 'a' [module-path-mismatch]
$ echo $?
1
```

No último caso, `gen/` não é criado.

---

## 9. Plano de implementação

Cada marco termina com o seu teste passando e o anterior intacto.

| Marco | Entrega | Aceitação |
| --- | --- | --- |
| **M0** driver | `args.c`, modo transparente, `--cgen-version`/`--cgen-help`, erros de invocação, resolução da base | tabela de `argv` → (opções do cgen, repasse, fonte) em teste de unidade; §8.1, §8.2, e os três primeiros de §8.8 |
| **M1** lexer | `lexer.c` + `--stop-after=lex` | os casos do [lexer-design §8](lexer-design.md#8-casos-de-aceitação) e §8.4; toda a `/base` e todo `.k` de `golden/cases` lexam sem diagnóstico |
| **M2** módulos | `paths.c`, `tool.c`, parser de nível de arquivo (`module`, `import`, `import_c`, `extern_c`, assinaturas), `--stop-after=parse` sem ilhas | raízes, `missing-module`, `module-path-mismatch`, `circular-import`, `module-not-found`, `generic-source-without-instance`; §8.5 até `decl` |
| **M3** geração sem ilhas | `emit.c`, `writer.c`, `#line`, mangling de nível de arquivo, `--main` | §8.3 byte a byte; segunda execução não muda `mtime` |
| **M4** cc e depfile | `cc.c`, `depfile.c`, critério de atualização | §8.6; editar `geom.k` faz `main` regerar os headers de `geom` e não escrever `gen/geom.c` |
| **M5** base | módulos genéricos, instâncias de modificador embutido, `--instance`, despacho de builtin, `defer` | `golden/cases/001`; §8.7 |
| **M6…** construções | uma por vez, na ordem dos casos golden | o caso correspondente |

## 10. Testes

- **Unidade** (`tools/cgen/test/`): partição de `argv`, normalização de
  caminho, mangling, lexer. Um executável por área, sem framework.
- **Golden**: para cada caso e perfil, `cgen --stop-after=gen --profile=<p>
  --base-dir base -I golden/cases/<c> --dest-dir <tmp> golden/cases/<c>/<m>.k`
  e `diff -r` contra `expected/<p>` (os arquivos do módulo) e contra
  `golden/<p>/` (os da base e do prelúdio). O `run.sh` existente continua
  provando que o esperado compila e se comporta; este passo prova que o cgen o
  produz. Enquanto o golden tiver prosa explicativa nos cabeçalhos (**[P5]**), o
  diff ignora o comentário de abertura de cada arquivo.
- **Determinismo**: gerar duas vezes em diretórios diferentes, com a ordem de
  `-I` preservada, e comparar byte a byte.

---

## 11. Bootstrap

`tools/cgen/Makefile`, alvo `base`, constrói o `transform` e gera em
`tools/cgen/gen/` os tipos de apoio (§3.2). Na distribuição, `lib/src` leva esse
`gen/` pronto, para que o usuário compile o cgen sem o transform (spec §8). Na
versão 2, o cgen é escrito em keel e compila a si mesmo sobre a `/base`; o
transform deixa de ser necessário.

---

## 12. Decisões deste documento

| | Decisão | Por quê |
| --- | --- | --- |
| D2 | Opção longa com valor aceita `--x=v` e `--x v` | a tabela da spec usa as duas grafias em opções diferentes; aceitar ambas em todas evita que o usuário decore qual é qual |
| D3 | `--main` sem `-c` compila a unidade de entrada na mesma chamada; com `-c`, só gera | `cgen --main m m.k -o prog` tem de dar executável; `-c -o` com dois `.c` é erro do gcc |
| D6 | `-I D` vai no fim da linha do `cc` | as raízes do usuário mantêm precedência; a spec §4.4 põe `-I gen` no meio, mas o exemplo é ilustrativo e o efeito é o mesmo |
| D7 | A classe de token em `--stop-after=lex` é calculada sobre a grafia lógica | é o que o parser vê; imprimir `ident` para `ret\`+`urn` esconderia justamente a emenda que se quer depurar |
| D8 | O fonte se parte em `tool/` e `engine/`, num executável só | a spec da ferramenta §3 já separa E/S de tradução por responsabilidade; o diretório é o que torna a fronteira verificável por grep, em vez de por leitura |

(D1, D4 e D5 da primeira versão — o `.` padrão de `-I`, `base-not-found`
e o código de saída do `cc` — subiram para a spec da ferramenta.)

## 13. Pendências nos normativos

**Nenhuma pendência em aberto.**

O que este documento levantou antes, enquanto era escrito, já está nos
normativos; o registro abaixo fica para que cada decisão seja rastreável até
onde ela mora hoje.

Resolvida em 2026-09-21: P16 (visibilidade do C que atravessa o módulo). `extern_c` passa a seguir `pub`/`priv` como qualquer construção de arquivo; o modificador `[type_h]` direciona o conteúdo para `.type.h`; `extern_c` sem `[type_h]` vai para `.h`; `priv extern_c` vai para `.c`; `priv extern_c [type_h]` é erro (`type-layer-on-priv-extern-c`). Diretivas de topo vão para `.h`; dentro de construto, seguem o construto. Spec §4.1, backend §4.1.

Resolvida em 2026-09-20: P15 (golden derivado da `/base` — `#line` em toda
declaração levada a header, prosa fora, instâncias inteiras pela regra do
backend §7.2, `arena` e `routine` seguindo o `.k`). Na mesma rodada, e por
causa dela: a instanciação degenerada da spec §4.3, o `verb-not-in-instance`
da §4.4, o sufixo de aridade do backend §2.1 e o `keel_arena_alloc` do §5.4.

Resolvidas na rodada de 2026-09-18: P1 (prelúdio na raiz), P2 (regra de padrão
da §4.10), P3 (`--base-dir`, sem `KEEL_HOME`), P8 (o `.c` só da invocação), P9
(a base não vai ao `cc`), P10 (lexer-design), P11 (`prelude.h` no backend §3.2),
P6 (unidade de entrada inclui só o `.h`), P7 (código do `cc` na spec §7, era D5)
e P12 (`base/` no repositório, `lib/base/` na distribuição).
Resolvida em 2026-09-19: P13 (golden no corte em dois, `.type.h` + `.h`); P4 e P5 nos módulos dos
casos (`import_c` no `.type.h` com `#line`, `#line` em toda declaração levada a
header, mapeamento de linha 1:1 nos `.c`, tag sintética, prosa e nomes que não
vêm do fonte, genérico sem `.c`); P14 (parâmetro de tipo opaco no genérico,
`protocol-on-parameter`, spec §4.3).
