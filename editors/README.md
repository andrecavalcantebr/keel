# editors — realce de sintaxe

Duas extensões, geradas a partir de `keel-spec.md`:

    python3 editors/gerar.py

**A lista de palavras não é digitada aqui** — o gerador lê a tabela do §3.7 do
documento. Se a spec ganhar ou perder uma palavra, rodar o gerador é o que
mantém os editores em dia, e é por isso que ele existe em vez de dois arquivos
escritos à mão.

## VSCode — dois arquivos, e a razão é medida

A intuição de "incluir o C e acrescentar as palavras" está certa, mas **não
basta um arquivo**. Com `"patterns": [{"include": "#keel"}, {"include":
"source.c"}]`, os padrões de keel não alcançam o interior das funções: a regra
de bloco do `source.c` assume e só as sub-regras dela valem ali. Tokenizando um
caso real, saía isto:

| Escrito | Escopo |
| --- | --- |
| `defer` | `meta.block.c` — sem realce |
| `foreach` | `entity.name.function.c` — realçado como chamada de função |
| `now` | `meta.bracket.square.access.c` — sem realce |
| `buffer` em corpo | `variable.other.object.access.c` |

Ou seja, justo onde mora quase toda palavra de keel. A forma certa é **injeção**:

- `keel.tmLanguage.json` — a base, que é só `{"include": "source.c"}`;
- `keel-injection.tmLanguage.json` — `injectionSelector: "L:source.keel
  -comment -string"`, que entra em **todo escopo aninhado**.

As duas exclusões do seletor também são medidas: sem `-comment -string`,
`defer` num comentário e `buffer` dentro de uma string saíam realçados.

E o intervalo precisou de um padrão a mais. Em `0..4`, a regra numérica do C
casa a partir do `0` e engole o `..` como número malformado
(`invalid.illegal.constant.numeric`); casando o operando esquerdo junto, a
injeção vence por posição.

Para instalar em desenvolvimento: copiar `editors/vscode/` para
`~/.vscode/extensions/keel/` e recarregar a janela.

## Zed — como instalar

Zed não tem comando de linha para isso: a instalação é pela interface.

1. Abrir a paleta de comandos e rodar **`zed: install dev extension`** — ou ir à
   página de extensões e clicar em **Install Dev Extension**.
2. Escolher o diretório **`editors/zed/`** deste repositório.
3. Abrir um `.k`. A linguagem aparece como `keel` no seletor do rodapé.

Na primeira instalação o Zed **baixa e compila o `tree-sitter-c`** fixado no
`extension.toml`, o que exige rede e o `wasi-sdk` — que o próprio Zed busca
sozinho. Se você já tiver um, `WASI_SDK_PATH` apontando para a raiz dele (a que
contém `bin/clang`) evita o download.

**Não é preciso ter Rust.** A extensão não tem `Cargo.toml` nem `src/lib.rs`:
ela só declara uma linguagem e uma gramática, então o `wasm32-wasip2` do
`rustup` — exigido por extensões com código próprio — não entra aqui.

Se algo falhar, `zed: open log` mostra o erro; `zed --foreground` a partir do
terminal mostra mais.

> **Uma ressalva.** A documentação do Zed diz que uma extensão *é um repositório
> Git*, e aqui ela é um subdiretório de um. Se o instalador recusar
> `editors/zed/`, o contorno é copiar esse diretório para um lugar próprio e
> apontar para lá — nada no conteúdo muda.

Para atualizar depois de mexer no `highlights.scm`: `zed: reload extensions`.

## Zed — o que ele consegue, e por que menos que o VSCode

Zed usa Tree-sitter, que é parser e não regex. **Não existe "incluir" o C por
configuração.** Há dois caminhos, e este repositório traz o barato:

**O que está aqui:** a linguagem declara `grammar = "c"` e traz um
`highlights.scm` próprio, que casa as palavras de keel **por grafia** sobre nós
`identifier` e `type_identifier`. Funciona bem na massa do arquivo, que é C. Nas
construções que o parser do C não reconhece — `foreach (i32 v : xs)`,
`parallel p ANY (…)`, `buffer i32 xs` — o Tree-sitter produz nós de erro e o
realce degrada. As palavras continuam realçadas; a estrutura em volta, nem
sempre.

O problema de comentário e string não se repete aqui: o parser põe comentário e
literal em nós próprios, e não há `identifier` dentro deles para a consulta
casar.

**O caminho certo, quando incomodar:** `tree-sitter-keel` estendendo
`tree-sitter-c`. Tree-sitter suporta herança de gramática — é como
`tree-sitter-cpp` é feito —, então a forma é a mesma que o VSCode consegue por
injeção, só que escrita em `grammar.js`. É projeto à parte, não arquivo de
configuração.

## O que foi verificado, e o que não foi

**VSCode: medido.** `editors/verifica.mjs` carrega a gramática real do C do
VSCode, aplica a injeção e tokeniza todos os `.k` de `golden/casos/`. O
cabeçalho do arquivo tem como rodar.

    176 ocorrências de palavra keel, 0 sem realce

Verificado também o inverso — nada realçado dentro de comentário ou string — e
que `x.foreach` continua sendo acesso a campo, não palavra.

**Zed: manifesto conferido, realce não medido.** O `extension.toml` e o
`config.toml` foram validados como TOML e comparados campo a campo com uma
extensão de linguagem real do repositório do Zed e com
`docs/src/extensions/languages.md` — foi assim que o `block_comment` saiu da
forma de lista, que é antiga, para a de tabela. O `commit` da gramática é a tag
v0.24.2, resolvida com `git ls-remote --tags`.

O que **não** foi medido é o resultado na tela: a instalação de extensão de
desenvolvimento é pela interface do Zed, e não há comando de linha para
automatizá-la.

## Nota sobre `.k`

O sufixo colide com a linguagem K em alguns ambientes. Se atrapalhar, o campo a
mudar é `extensions` no `package.json` e `path_suffixes` no `config.toml`.
