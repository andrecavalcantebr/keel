# diag — desenho do diagnóstico

> © 2026 Faculdade de Engenharia Elétrica e de Computação, Universidade
> Federal do Amazonas (FEEC/UFAM). Licenciado sob
> [CC BY-SA 4.0](../LICENSE-DOCS.md) ([tradução](../LICENSE-DOCS.pt.md)) —
> plano de implementação, não contrato normativo; o `cgen` e a Base keel têm
> licença própria, ver [`LICENSE.md`](../LICENSE.md). Escrita e revisão
> tiveram auxílio de Claude Opus (Anthropic), sob direção humana.

**Documento de implementação.** Cobre `engine/diag.c` e `tool/report.c` da
[`cgen-tool.md`](cgen-tool.md) §3.1. O contrato é
[`keel-spec.md`](../keel-spec.md) §6 — o catálogo e as obrigações por
severidade — e [`cgen-tool-spec.md`](../cgen-tool-spec.md) §7 — o formato, os
códigos de saída e o filtro de `-W`.

Onde **decide** algo que os normativos deixam aberto, a decisão vem marcada
**[G*n*]** e listada na §8.

---

## 1. O que falta hoje, e por que ele vem antes dos testes

O catálogo da spec §6.2 tem 135 identificadores, e para cada um dá a condição, a
severidade, o responsável e a seção. **Não dá o texto da mensagem.** Nenhuma
mensagem de diagnóstico do keel existe escrita em lugar nenhum do repositório.

Isso trava mais do que parece:

- **Os casos de falha do golden não podem ser escritos.** Um caso que afirme
  "este fonte é recusado" precisa afirmar *o quê* foi dito, senão passa com
  qualquer erro, inclusive o errado.
- **A qualidade da recusa é metade do valor da linguagem.** keel recusa muita
  coisa de propósito — `not-a-container-expression`, `symbol-redeclaration`,
  `verb-not-in-instance` — e uma recusa que não explica o que fazer em seguida é
  pior do que o erro de tipo do C que ela substituiu.

Daí a ordem: o catálogo de mensagens vem antes dos casos de falha, e este
documento antes dos dois.

---

## 2. A separação em dois

A fronteira `tool/`/`engine/` (desenho do cgen §3.1, D8) corta o diagnóstico ao
meio, e o corte é natural:

| | Depende de | Decide |
| --- | --- | --- |
| `engine/diag.c` | só do fonte | qual é a condição, qual a severidade, onde no `.k` |
| `tool/report.c` | da invocação | a ordem entre módulos, o filtro de `-W`, a cor, o destino |

O motor **acumula**, não imprime:

```c
typedef struct {
    KDiagId          id;        /* índice na tabela, não string       */
    KSeverity        severity;  /* da tabela; -Werror não muda aqui   */
    keel_slice_char  at;        /* fatia do fonte — posição e extensão */
    keel_slice_char  module;    /* qual .k                            */
    KDiagArgs        args;      /* os buracos da mensagem             */
    KDiagNote       *notes;     /* contexto: nota de instanciação etc. */
} KDiagnostic;

void k_diag_emit(KDiagnosticSink *s, KDiagId id, keel_slice_char at, KDiagArgs args);
size_t k_diag_count(const KDiagnosticSink *s, KSeverity sev);
```

**A severidade que o motor registra é a da tabela.** `-Werror` é da invocação, e
quem o aplica é o `report.c` ao decidir o código de saída — se o motor a
mudasse, o mesmo fonte daria diagnósticos diferentes conforme a linha de
comando, e o motor deixaria de ser função só do fonte.

**A posição é uma fatia, não linha e coluna.** Converter custa varrer o fonte
contando quebras, e diagnóstico é raro. O `report.c` converte na hora de
imprimir — e é ele quem sabe que a coluna conta **bytes** desde o início da
linha física (desenho do cgen §4).

---

## 3. A tabela

> **[G1] O catálogo é uma tabela estática no binário, gerada a partir da spec.**

```c
typedef struct {
    const char *name;       /* kebab-case, o do catálogo   */
    KSeverity   severity;
    const char *fmt;        /* a mensagem, com os buracos  */
} KDiagEntry;

static const KDiagEntry k_diags[] = {
    [K_DIAG_MISSING_MODULE] = { "missing-module", K_ERROR,
        "file does not begin with a `module` declaration" },
    [K_DIAG_MODULE_PATH_MISMATCH] = { "module-path-mismatch", K_ERROR,
        "module `%s` should be declared in `%s`, not `%s`" },
    …
};
```

Ordenada por nome, para que o filtro de `-W` seja busca binária (desenho do
cgen §2.3). `KDiagId` é índice, então o motor nunca compara string.

**Gerada, não escrita à mão.** A spec §6.2 já é uma tabela markdown com
identificador e severidade; um script extrai as duas colunas e confere contra
o `.h`. O que o script **não** extrai é a mensagem — essa é escrita à mão, e o
script verifica apenas que existe uma para cada identificador, e que não sobra
nenhuma para identificador que o catálogo não tem.

Isso dá, de graça, a verificação que hoje eu faço com `grep`: catálogo e
implementação não podem divergir sem que o build quebre.

---

## 4. Como se escreve uma mensagem

> **[G2] A mensagem diz o que foi encontrado, o que era esperado, e — quando há
> uma — a forma correta.**

Não é estilo; é o que separa uma recusa útil de uma recusa que manda o
programador reler a spec. Quatro regras, com o exemplo de cada uma:

**Cite o que o programa escreveu.** O nome, a grafia, a aridade — não a regra.

```plain
ruim:  argument arity does not match the modifier declaration
bom:   `buffer` takes 1 type argument, but 2 were written
```

**Dê a forma correta quando ela é única.** Vários diagnósticos existem
precisamente porque há uma grafia certa e uma errada; escondê-la é desperdiçar
a recusa.

```plain
import-clause-order:   write `import M as m types;` — `as` comes before `types`
buffer-over-const:     `buffer` cannot own const storage; use `slice const T`
c-type-as-argument:    write `i32`, not `int`, as a modifier argument
```

**Nomeie o outro lado quando houver um.** Colisão, sombreamento e ciclo têm
dois participantes, e citar só um deixa o trabalho pela metade.

```plain
duplicate-alias:   alias `m` is already bound to `net.http` (imported at line 4)
circular-import:   app.cfg → app.db → app.cfg
```

**Não explique a regra.** A mensagem cabe numa linha e aponta o catálogo pelo
nome entre colchetes; quem quer o porquê tem a spec e o rationale.

### 4.1 As notas

`note:` carrega o que não cabe na linha e não é a condição em si. Dois usos
previstos:

- **A nota de instanciação** (backend §6.1), que diz qual instanciação quebrou
  quando o erro é do compilador C dentro de um genérico. É a única informação
  que o `#line` sozinho não alcança.
- **O outro participante**, quando a posição dele é longe: a declaração
  anterior numa colisão, o import que trouxe o alias.

---

## 5. As três famílias de mensagem

O catálogo se divide em três, e cada uma tem uma forma própria — o que ajuda a
escrever as 135 sem inventar um estilo por vez:

| Família | Quantos | Forma | Exemplo |
| --- | --- | --- | --- |
| **Grafia errada, forma certa conhecida** | ~40 | "escreva X, não Y" | `array-1d-as-parameter`, `enum-constant-without-type`, `import-clause-order` |
| **Conflito entre dois pontos do programa** | ~25 | "X já é Y, declarado em Z" | `symbol-collision`, `duplicate-alias`, `canonical-name-collision` |
| **O tipo não oferece o que a construção pede** | ~35 | "T não declara V" + o que declara | `not-partitionable`, `no-range-index-verb`, `verb-not-in-instance` |

A terceira é a mais valiosa e a mais fácil de escrever mal. O protocolo é uma
lista de verbos (spec §5.1), então a mensagem pode dizer exatamente o que falta:

```plain
error: `mylist` cannot be used with `parallel`: it does not declare `partition`
note: `parallel` needs `partition(x, k, w)`; see keel-spec §5.1
```

E `verb-not-in-instance` tem informação que nenhum compilador C teria, e que a
mensagem deve gastar:

```plain
error: `set` is not available on `slice const char` [verb-not-in-instance]
note: the `const` argument removes the verbs that write through it
```

---

## 6. A impressão

`tool/report.c`, no formato do gcc (spec da ferramenta §7):

```plain
<arquivo>:<linha>:<coluna>: <severidade>: <mensagem> [<nome>]
```

Cinco regras da implementação:

1. **Ordem de fonte por módulo, ordem de carga entre módulos.** Nada de ordem
   de hash — é a mesma disciplina do determinismo do gerado.
2. **Linha e coluna começam em 1; a coluna conta bytes** desde o início da
   linha física, que é o que o gcc faz com
   `-fdiagnostics-column-unit=byte`.
3. **Toda posição é no `.k`**, nunca no gerado. Um diagnóstico que apontasse
   para `gen/` seria bug.
4. **Os dois espaços de nome não se cruzam** (spec da ferramenta §7.1): um
   identificador da ferramenta nunca aparece no catálogo da linguagem, e é o
   que permite ao filtro de `-W` decidir olhando uma lista só.
5. **`-Wno-<nome>` de nome que não é do keel atravessa para o `cc`.** Opção
   `-W` desconhecida é **erro** no gcc, não aviso, e derrubaria o build.

### 6.1 Códigos de saída

| Código | Situação |
| --- | --- |
| 0 | sucesso, com ou sem `warning` |
| 1 | pelo menos um `error` da linguagem; nada foi escrito |
| 2 | erro da ferramenta (spec da ferramenta §7.1) |
| *n* | o compilador C saiu com *n*; por sinal *s*, 128+*s* |

`-Werror` entra aqui: com ele, `k_diag_count(sink, K_WARNING) > 0` também dá 1.

---

## 7. Os casos de falha

O golden hoje tem **zero** casos de recusa — o runner conta `xfail` e nunca viu
um. A meta é um caso por identificador, e escrever 135 casos não é viável nem
desejável.

> **[G3] Um caso agrupa vários diagnósticos independentes; o `VERIFY` afirma
> quais saíram, lendo os marcadores do próprio fonte.**

O molde já existe: o caso 016 não compila de propósito e afirma as posições
**sem escrever número nenhum**, lendo as linhas marcadas `/* ERROR */` no `.k`.
A extensão natural marca também o identificador esperado:

```keel
module bad.names;
import net.http as m;
import net.mail as m;          /* DIAG: duplicate-alias */

pub int f(void) {
    i32 buffer;                /* DIAG: keel-name-shadowed */
    return 0;
}
```

E o `VERIFY` extrai o esperado do fonte, roda o cgen, e compara os conjuntos:

```sh
want=$(grep -o 'DIAG: [a-z-]*' "$dir/bad/names.k" | cut -d' ' -f2 | sort)
got=$(cgen --dest-dir "$tmp" "$dir/bad/names.k" 2>&1 |
      grep -o '\[[a-z-]*\]' | tr -d '[]' | sort)
[ "$want" = "$got" ]
```

Três propriedades disso:

- **O teste não repete o esperado.** O fonte é a única fonte de verdade, como no
  016. Mover uma linha não quebra o teste.
- **Agrupa sem confundir.** Os diagnósticos têm que ser independentes — um que
  faça o parser ressincronizar (parser §3.1) pode engolir o seguinte, e aí os
  dois não cabem no mesmo caso.
- **Cobertura é verificável.** Um script sobre todos os `.k` de falha lista os
  identificadores cobertos e os compara com o catálogo. É o que diz quantos dos
  135 faltam, a qualquer momento.

**O que não cabe nesse molde:** os 7 `debug`, que são verificações em execução e
não recusas de tradução. Esses precisam de caso que **compile**, rode sob
`--checks=on`, e afirme o `abort`. É outra família, e o runner precisa de um
modo para ela.

---

## 8. Decisões deste documento

| | Decisão | Por quê |
| --- | --- | --- |
| G1 | O catálogo é tabela estática gerada a partir da spec; só a mensagem é escrita à mão | catálogo e implementação não podem divergir sem quebrar o build, e hoje isso só se verifica com `grep` |
| G2 | A mensagem diz o encontrado, o esperado e a forma correta — não a regra | keel recusa muito de propósito; recusa que não diz o que fazer em seguida é pior que o erro de tipo do C que ela substituiu |
| G3 | Um caso de falha agrupa vários diagnósticos; o `VERIFY` lê o esperado do próprio fonte | 135 casos não é viável; e o molde do 016 já prova que dá para afirmar sem duplicar o esperado no teste |
| G4 | A severidade registrada é a da tabela; `-Werror` só afeta o código de saída | se o motor a mudasse, o mesmo fonte daria diagnósticos diferentes conforme a linha de comando |

---

## 9. Pendências

| | Onde | Divergência |
| --- | --- | --- |
| R1 | spec §6.1 × este documento | a severidade `info` tem duas entradas no catálogo (`injected-names`, `indirect-import`) e a spec diz que "a apresentação é definida pela ferramenta" — mas não diz se `info` sai por padrão. Sair sempre é ruído; nunca sair torna a entrada decorativa. O candidato é ligar por `-W<nome>`, e falta decidir |
| R2 | golden × este documento | o runner não tem modo para caso `debug`: compilar, rodar sob `--checks=on` e afirmar o `abort`. Os 7 `debug` do catálogo ficam sem teste até ele existir |
