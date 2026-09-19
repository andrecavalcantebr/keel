# cgen — desenho da implementação

> © 2026 Faculdade de Engenharia Elétrica e de Computação, Universidade
> Federal do Amazonas (FEEC/UFAM). Licenciado sob
> [CC BY-SA 4.0](../LICENSE-DOCS.md) ([tradução](../LICENSE-DOCS.pt.md)) —
> plano de implementação, não contrato normativo; o `cgen` e a Base keel têm
> licença própria, ver [`LICENSE.md`](../LICENSE.md). Escrita e revisão
> tiveram auxílio de Claude Opus (Anthropic), sob direção humana.

**Documento de implementação.** O contrato da ferramenta é
[`cgen-tool-spec.md`](../cgen-tool-spec.md) (a "spec da ferramenta"); o do
conteúdo gerado, [`keel-c-backend.md`](../keel-c-backend.md) (o "backend"); o do
lexer, [`lexer-design.md`](lexer-design.md). Este documento não acrescenta
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
**[D2]**. Valor fora do enumerado é `opcao-invalida` (código 2). Opção que
exige valor e está no fim da linha também é `opcao-invalida`.

Repetição: a última ocorrência vence, salvo `-I`, que acumula. `--instance`
repetido é `fonte-multiplo`.

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
    mais de um → fonte-multiplo (código 2)
```

Uma palavra que é `-I` vale para os dois lados: entra nas raízes **e** no
repasse, na mesma posição. O `.` padrão só existe quando nenhum `-I` é dado:
se fosse sempre raiz, `cgen -I src src/a.k` seria sempre
`fonte-em-varias-raizes`, porque `src/a.k` está sob `.` e sob `src`.

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

Um `.k` de módulo genérico sem `--instance` é `fonte-generico` (código 2),
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

Nenhuma das duas contendo `keel.k`: `base-nao-encontrada`. Sem variável de
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
   `modulo-nao-encontrado`. Achado e não genérico, ou sem esse modificador:
   `instancia-invalida`.
4. Os argumentos são resolvidos como num uso, carregando cada módulo que
   qualificam. Aridade ou espécie que não casa com `dim`/`tags`/`type` da linha
   `module`: `instancia-invalida`.
5. A invocação compila a instância: os três headers dela e o `.c` com o símbolo
   dela (`gen/keel/keel_buffer_i32.c`). Genérico todo `pub inline`:
   `instance-inutil` (`warning`), e o `.c` sai só com o include.

A identidade da invocação é o símbolo da instância — é ele que nomeia o `.c`, o
`.o` derivado e o alvo do depfile.

---

## 3. Arquitetura do código

### 3.1 Arquivos

Em `tools/cgen/src/`, um par `.c`/`.h` por responsabilidade:

| Arquivo | Responsabilidade | Não faz |
| --- | --- | --- |
| `main.c` | `argv` → `KInvocation`; escolhe o modo | nada de E/S além de chamar os outros |
| `args.c` | partição (§2.5), validação de valores | não resolve caminho |
| `paths.c` | normalização, raiz de um fonte, módulo ↔ caminho `.k`, símbolo ↔ gerado, executável real (§2.7) | não abre arquivo |
| `tool.c` | `carrega`/`processa` (spec §3), pilha de carga, memoização, fecho de mtime | não parseia |
| `lexer.c` | [`lexer-design.md`](lexer-design.md) | — |
| `parser.c`, `symtab.c` | ilhas, declarações, tabela de símbolos | não abre arquivo, não escreve |
| `emit.c` | backend: `.type.h`, `.proto.h`, `.h` de tudo que é alcançado; `.c` só do compilado | não escreve em disco |
| `writer.c` | spec da ferramenta §6: comparar, temporário, `rename` | não gera conteúdo |
| `depfile.c` | §7 | — |
| `cc.c` | monta o `argv` do `cc`, `posix_spawnp` + `waitpid` | — |
| `diag.c` | `KDiagnosticSink`, formatação, filtros `-W`, contagem de `error` | não termina o processo |

Só `main.c` chama `exit`. Parser e lexer não fazem E/S (spec §3); o parser
chama de volta `tool.c` por ponteiro de função:

```c
typedef enum { K_LOAD_OK, K_LOAD_ALREADY, K_LOAD_NOT_FOUND, K_LOAD_ERROR } KLoadResult;

typedef struct KModule KModule;            /* símbolos públicos, imports, mtime do fecho */

typedef struct {
    KLoadResult (*load)(void *tool, keel_slice_char module_name, KModule **out);
    void *tool;
} KLoader;

/* processa(fonte, nome) → artefatos, diagnósticos */
bool k_process(keel_slice_char source, keel_slice_char module_name, bool compiled,
               KLoader *loader, KDiagnosticSink *diag, KArtifacts *out);
```

`compiled` diz se o módulo é o da invocação: só então `KArtifacts` recebe o
`.c`. `K_LOAD_ALREADY` inclui o caso "em carga": `tool.c` sabe que o módulo está
na pilha e devolve o erro de ciclo com a cadeia; o parser emite
`import-circular` na posição do `import`.

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
   lexicalmente); acha a **única** raiz que o contém (`fonte-fora-de-raiz`,
   `fonte-em-varias-raizes`). O nome esperado do módulo é o caminho relativo à
   raiz, sem `.k`, com `/` → `.`. **Com `--instance`:** §2.8.
5. `processa` o fonte com `compiled = true`. O `module` declarado tem de bater
   com o nome esperado (`module-fora-do-caminho`, da linguagem, código 1).
   Módulo genérico sem `--instance`: `fonte-generico`.
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
- Erro de ferramenta sem posição: `cgen: error: <mensagem> [<nome>]`.

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
| módulo compilado `a.b` | `D/a/a_b.type.h`, `D/a/a_b.proto.h`, `D/a/a_b.h`, `D/a/a_b.c` |
| módulo importado `a.b` | `D/a/a_b.type.h`, `D/a/a_b.proto.h`, `D/a/a_b.h` |
| o prelúdio `keel`, sempre importado | `D/keel.type.h`, `D/keel.proto.h`, `D/keel.h` |
| genérico importado `keel.buffer` | `D/keel/keel_buffer.{type,proto}.h`, `D/keel/keel_buffer.h` — só o que não menciona parâmetro (backend §4.4.1) |
| instância usada `buffer i32` | `D/keel/keel_buffer_i32.{type,proto}.h`, `D/keel/keel_buffer_i32.h` |
| instância compilada por `--instance` | os três headers e `D/keel/keel_buffer_i32.c` |
| `--main a.b` | `D/main_a_b.c` |

Todo arquivo gerado começa com uma linha de cabeçalho, sem versão nem data
(spec §6.1):

```c
/* <caminho relativo a D> — gerado de <caminho do .k relativo à raiz> pelo cgen, perfil C23. */
```

O include guard é o caminho relativo a `D` em maiúsculas, com todo caractere
fora de `[A-Z0-9]` virando `_`: `app/app_cfg.proto.h` → `APP_APP_CFG_PROTO_H`.

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
$ cgen --version          # é do cc
gcc (GCC) 14.2.1 …
```

### 8.2 Link transparente

```sh
$ cgen main.o geom.o -o prog -lm
```

Sem fonte: o cgen executa `cc main.o geom.o -o prog -lm` e sai com o código
dele. Nada é gerado e `gen/` não é criado.

### 8.3 Olá, com ponto de entrada

`ola.k`:

```keel
module ola;
import_c <stdio.h>;

pub int main(int argc, char **argv) {
    puts("oi");
    return 0;
}
```

```sh
$ cgen -std=c23 -Wall --main ola ola.k -o ola
```

Raiz `.` (nenhum `-I`); módulo esperado `ola`, declarado `ola`. Perfil C23
por `-std=c23`. Arquivos escritos:

```plain
gen/keel.type.h  gen/keel.proto.h  gen/keel.h            (keel.k, importado)
gen/ola.type.h   gen/ola.proto.h   gen/ola.h   gen/ola.c (ola.k, compilado)
gen/main_ola.c
```

`gen/ola.type.h` — `import_c` vai na camada mais baixa, com `#line` (backend
§4.1 e §6):

```c
/* ola.type.h — gerado de ola.k pelo cgen, perfil C23. */
#ifndef OLA_TYPE_H
#define OLA_TYPE_H
#include "keel.type.h"
#line 2 "ola.k"
#include <stdio.h>
#endif /* OLA_TYPE_H */
```

`gen/ola.proto.h` — o protótipo é declaração levada a header, e ganha `#line`
com a linha dela:

```c
/* ola.proto.h — gerado de ola.k pelo cgen, perfil C23. */
#ifndef OLA_PROTO_H
#define OLA_PROTO_H
#include "ola.type.h"
#line 4 "ola.k"
int ola_main(int argc, char **argv);
#endif /* OLA_PROTO_H */
```

`gen/ola.h` — nenhum corpo `inline`, então só o include:

```c
/* ola.h — gerado de ola.k pelo cgen, perfil C23. */
#ifndef OLA_H
#define OLA_H
#include "ola.proto.h"

#endif /* OLA_H */
```

`gen/ola.c` — o corpo é copiado com as quebras intactas; `pub` sai e o nome
recebe o prefixo, na mesma linha:

```c
/* ola.c — gerado de ola.k pelo cgen, perfil C23. */
#include "ola.h"
#line 4 "ola.k"
int ola_main(int argc, char **argv) {
    puts("oi");
    return 0;
}
```

`gen/main_ola.c` — inclui o `.h` do módulo, como qualquer `.c` (regra 4 do
backend §4.3.2):

```c
/* main_ola.c — unidade de entrada, gerada por --main ola. */
#include "ola.h"
int main(int argc, char **argv) { return ola_main(argc, argv); }
```

A chamada ao compilador C:

```sh
cc -std=c23 -Wall gen/ola.c gen/main_ola.c -o ola -I gen
```

Uma segunda execução idêntica: tudo está em dia, o cgen reparseia (para ter
os símbolos), não escreve nada — nenhum `mtime` muda — e chama o `cc` de novo,
porque ligar é o que foi pedido.

### 8.4 `--stop-after=lex`

```sh
$ cgen --stop-after=lex ola.k | head -12
ola.k:1:1: ident "module"
ola.k:1:8: ident "ola"
ola.k:1:11: punct ";"
ola.k:2:1: ident "import_c"
ola.k:2:10: punct "<"
ola.k:2:11: ident "stdio"
ola.k:2:16: punct "."
ola.k:2:17: ident "h"
ola.k:2:18: punct ">"
ola.k:2:19: punct ";"
ola.k:4:1: ident "pub"
ola.k:4:5: cword "int"
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

`golden/casos/001-arena-buffer-defer`, a partir da raiz do caso:

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

(A lista de ilhas segue até o fim de `soma_scratch`; o teste compara a saída inteira.)

### 8.6 Compilação separada com depfile

`src/main.k` (`module main;`, importa `geom`), `src/geom.k` (`module geom;`):

```sh
$ cgen -I src -MMD -c src/main.k -o main.o -O2
```

Escreve `gen/main.{type,proto}.h`, `gen/main.h`, `gen/main.c`; os três headers
de `geom` se desatualizados — **nunca `gen/geom.c`** —; e os do prelúdio.
Executa:

```sh
cc -I src -MMD -c gen/main.c -o main.o -O2 -MF main.d.tmp.<pid> -I gen
```

e escreve `main.d`:

```make
main.o: gen/main.c gen/main.h gen/main.proto.h gen/main.type.h \
 gen/keel.type.h gen/geom.h gen/geom.proto.h gen/geom.type.h \
 src/main.k src/geom.k /opt/keel/lib/base/keel.k
```

`geom.o` tem regra própria (`cgen -I src -MMD -c src/geom.k -o geom.o`), e é
ela que escreve `gen/geom.c`.

### 8.7 Instância pré-compilada

```sh
$ cgen -std=c23 -c --instance "keel.buffer.buffer i32" -o keel_buffer_i32.o
cgen: warning: keel.buffer.buffer é inteiramente pub inline; o .c não terá corpo [instance-inutil]
```

Escreve `gen/keel/keel_buffer_i32.{type,proto}.h`, `gen/keel/keel_buffer_i32.h`,
`gen/keel/keel_buffer_i32.c` (só `#include "keel/keel_buffer_i32.h"`), os
headers de `keel.buffer`, `keel.slice`, `keel.outcome`, `keel.arena` e do
prelúdio. Executa `cc -std=c23 -c gen/keel/keel_buffer_i32.c -o
keel_buffer_i32.o -I gen`.

### 8.8 Erros

```sh
$ cgen a.k b.k
cgen: error: mais de um fonte na invocação [fonte-multiplo]
$ echo $?
2

$ cgen --profile=c99 ola.k
cgen: error: valor 'c99' inválido para --profile; esperado auto, c11 ou c23 [opcao-invalida]
$ echo $?
2

$ cgen -I . -I src src/a.k
cgen: error: src/a.k está sob mais de uma raiz: '.' e 'src' [fonte-em-varias-raizes]

$ cgen --base-dir base base/keel/buffer.k
cgen: error: keel.buffer é módulo genérico; use --instance [fonte-generico]

$ cgen --instance "keel.buffer i32"
cgen: error: 'keel.buffer' não nomeia modificador por inteiro; o módulo 'keel' não é genérico [instancia-invalida]

$ cat src/a.k
module b;
$ cgen -I src src/a.k
src/a.k:1:8: error: o módulo declara 'b', mas o caminho sob a raiz 'src' diz 'a' [module-fora-do-caminho]
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
| **M1** lexer | `lexer.c` + `--stop-after=lex` | os casos do [lexer-design §8](lexer-design.md#8-casos-de-aceitação) e §8.4; toda a `/base` e todo `.k` de `golden/casos` lexam sem diagnóstico |
| **M2** módulos | `paths.c`, `tool.c`, parser de nível de arquivo (`module`, `import`, `import_c`, `extern_c`, assinaturas), `--stop-after=parse` sem ilhas | raízes, `sem-module`, `module-fora-do-caminho`, `import-circular`, `modulo-nao-encontrado`, `fonte-generico`; §8.5 até `decl` |
| **M3** geração sem ilhas | `emit.c`, `writer.c`, `#line`, mangling de nível de arquivo, `--main` | §8.3 byte a byte; segunda execução não muda `mtime` |
| **M4** cc e depfile | `cc.c`, `depfile.c`, critério de atualização | §8.6; editar `geom.k` faz `main` regerar os headers de `geom` e não escrever `gen/geom.c` |
| **M5** base | módulos genéricos, instâncias de modificador embutido, `--instance`, despacho de builtin, `defer` | `golden/casos/001`; §8.7 |
| **M6…** construções | uma por vez, na ordem dos casos golden | o caso correspondente |

## 10. Testes

- **Unidade** (`tools/cgen/test/`): partição de `argv`, normalização de
  caminho, mangling, lexer. Um executável por área, sem framework.
- **Golden**: para cada caso e perfil, `cgen --stop-after=gen --profile=<p>
  --base-dir base -I golden/casos/<c> --dest-dir <tmp> golden/casos/<c>/<m>.k`
  e `diff -r` contra `esperado/<p>` (os arquivos do módulo) e contra
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

(D1, D4 e D5 da primeira versão — o `.` padrão de `-I`, `base-nao-encontrada`
e o código de saída do `cc` — subiram para a spec da ferramenta.)

## 13. Pendências nos normativos

| | Onde | Divergência |
| --- | --- | --- |
| P4 | golden × backend §4.1/§6 | o golden põe `import_c` no `.c`, sem `#line`, e não põe `#line` nas declarações levadas a header; o backend põe `import_c` no `.type.h` e `#line` nos dois. Resolve-se na auditoria dos casos |
| P5 | golden | comentários de abertura com prosa que o cgen não pode gerar; `003` tem `(void)argc; (void)argv;` que não está no fonte — comparação byte a byte falha até o golden ser regularizado |

Resolvidas na rodada de 2026-09-18: P1 (prelúdio na raiz), P2 (regra de padrão
da §4.10), P3 (`--base-dir`, sem `KEEL_HOME`), P8 (o `.c` só da invocação), P9
(a base não vai ao `cc`), P10 (lexer-design), P11 (`prelude.h` no backend §3.2),
P6 (unidade de entrada inclui só o `.h`), P7 (código do `cc` na spec §7, era D5)
e P12 (`base/` no repositório, `lib/base/` na distribuição).
