# editors — realce de sintaxe

Duas extensões, geradas a partir de `keel-spec.md`:

    python3 editors/gerar.py

**A lista de palavras não é digitada aqui** — o gerador lê a tabela do §3.7 do
documento. Se a spec ganhar ou perder uma palavra, rodar o gerador é o que
mantém os editores em dia, e é por isso que ele existe em vez de dois arquivos
escritos à mão.

## VSCode — herda a gramática do C, como se pretendia

TextMate é baseado em regex, e a gramática do C vem embutida no VSCode. Então a
de keel é literalmente "o C mais as palavras":

```json
"patterns": [ { "include": "#keel" }, { "include": "source.c" } ]
```

A ordem importa: keel primeiro, o C depois. Comentário e string continuam sendo
do C, porque quem casa a abertura deles é o `source.c` e ele consome até o fim.

Para instalar em desenvolvimento: copiar `editors/vscode/` para
`~/.vscode/extensions/keel/` e recarregar a janela.

## Zed — outra tecnologia, e por isso outro resultado

Zed usa Tree-sitter, que é parser e não regex. **Não existe "incluir" o C por
configuração.** Há dois caminhos, e este repositório traz o barato:

**O que está aqui:** a linguagem declara `grammar = "c"` e traz um
`highlights.scm` próprio, que casa as palavras de keel **por grafia** sobre nós
`identifier` e `type_identifier`. Funciona bem na massa do arquivo, que é C. Nas
construções que o parser do C não reconhece — `foreach (i32 v : xs)`,
`parallel p ANY (…)`, `buffer i32 xs` — o Tree-sitter produz nós de erro e o
realce degrada. As palavras continuam realçadas; a estrutura em volta, nem
sempre.

**O caminho certo, quando incomodar:** `tree-sitter-keel` estendendo
`tree-sitter-c`. Tree-sitter suporta herança de gramática — é como
`tree-sitter-cpp` é feito —, então a forma é a mesma que o VSCode consegue por
configuração, só que escrita em `grammar.js`. É projeto à parte, não arquivo de
configuração.

> **`extension.toml` tem um `commit = "PREENCHER"`.** Zed exige o commit fixado
> da gramática, e ele não foi inventado aqui. Preencher com um commit real de
> `tree-sitter-c` antes de instalar.

## O que foi verificado, e o que não foi

Verificado: o JSON dos três arquivos é válido, as cinco regexes do tmLanguage
compilam, e a lista de palavras sai da spec — 31 palavras, quatro grupos.

**Não verificado:** o resultado visual em nenhum dos dois editores. Nenhum está
instalado aqui, então o realce é derivado da spec e da documentação do formato,
não de um teste de tela.

## Nota sobre `.k`

O sufixo colide com a linguagem K em alguns ambientes. Se atrapalhar, o campo a
mudar é `extensions` no `package.json` e `path_suffixes` no `config.toml`.
