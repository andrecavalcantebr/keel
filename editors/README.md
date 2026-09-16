# editors — realce de sintaxe

Duas extensões, com o vocabulário keel gerado a partir de `keel-spec.md`:

    python3 editors/gerar.py

**A lista de palavras não é digitada aqui** — o gerador lê a tabela do §2.2 do
documento. Se a spec ganhar ou perder uma palavra, rodar o gerador é o que
mantém os editores em dia. No Zed, o gerador combina esse vocabulário com
`editors/c-highlights.scm`, que contém as regras de realce de C.

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

## VSCode — como instalar

```sh
./editors/instalar-vscode.sh              # link para editors/vscode/
./editors/instalar-vscode.sh --copiar     # cópia, para levar a outra máquina
```

O script cria `~/.vscode/extensions/<publisher>.<name>-<version>` a partir do
`package.json`. Por padrão é um **link simbólico**, e o VSCode segue link: rodar
`gerar.py` de novo atualiza a extensão instalada sem reinstalar. Depois:

1. Paleta de comandos → **`Developer: Reload Window`**.
2. Abrir um `.k`. A linguagem aparece como `keel` no rodapé.
3. Para conferir o escopo de uma palavra: **`Developer: Inspect Editor Tokens
   and Scopes`**.

`code --list-extensions | grep keel` confirma que o VSCode a enxergou. Outro
destino vai em `VSCODE_EXT`: `~/.vscode-server/extensions` para Remote-SSH,
`~/.vscode-oss/extensions` para VSCodium. Para desinstalar, basta apagar o link.

## Zed — como instalar

Zed não tem comando de linha para isso: a instalação é pela interface.

> **`editors/zed/` não pode ser instalado direto.** O Zed exige que uma extensão
> seja um **repositório Git**, e recusa um subdiretório de outro repositório —
> isso está nos docs dele e foi confirmado na prática. É o que o script abaixo
> resolve.

```sh
./editors/instalar-zed.sh                 # → ~/.local/share/keel-zed
./editors/instalar-zed.sh /outro/lugar
```

Ele copia `editors/zed/` para um diretório próprio e faz `git init` ali. Depois:

1. Abrir a paleta de comandos e rodar **`zed: install dev extension`** — ou ir à
   página de extensões e clicar em **Install Dev Extension**.
2. Escolher o diretório que o script criou.
3. Abrir um `.k`. A linguagem aparece como `keel` no seletor do rodapé.

Mexeu no `highlights.scm` ou rodou o `gerar.py`? Rode o script de novo e
`zed: reload extensions`.

Na primeira instalação o Zed **baixa e compila o `tree-sitter-c`** fixado no
`extension.toml`, o que exige rede e o `wasi-sdk` — que o próprio Zed busca
sozinho. Se você já tiver um, `WASI_SDK_PATH` apontando para a raiz dele (a que
contém `bin/clang`) evita o download.

**Não é preciso ter Rust.** A extensão não tem `Cargo.toml` nem `src/lib.rs`:
ela só declara uma linguagem e uma gramática, então o `wasm32-wasip2` do
`rustup` — exigido por extensões com código próprio — não entra aqui.

Se algo falhar, `zed: open log` mostra o erro; `zed --foreground` a partir do
terminal mostra mais.

**Por que `editors/` não mora fora do repositório do keel.** Seria a saída
óbvia para o problema do repositório próprio, e não funciona: `gerar.py` lê a
tabela do §2.2 de `keel-spec.md`, que fica ao lado. Fora daqui o gerador quebra,
e a lista de palavras volta a ser digitada à mão — que é exatamente o que ele
existe para evitar. Por isso a fonte fica no repositório e o que sai é uma
cópia.

## Zed — C e keel no mesmo arquivo

`grammar = "c"` seleciona o parser. As cores vêm das consultas em
`highlights.scm`, que a extensão precisa fornecer. A configuração anterior
continha somente as regras de keel; por isso palavras C como `if` e `return`,
números, strings e comentários ficavam sem suas capturas de realce.
A [documentação do Zed](https://zed.dev/docs/extensions/languages#syntax-highlighting)
descreve essa separação entre gramática e consultas.

O gerador agora reúne duas partes no mesmo `highlights.scm`:

- `editors/c-highlights.scm`: palavras C, tipos, funções, campos, literais,
  comentários, operadores, pontuação e diretivas;
- vocabulário da spec: palavras contextuais, tipos da camada zero e nomes da
  base keel, reconhecidos em nós `identifier` e `type_identifier`.

A regra genérica de nomes de tipo C exclui o vocabulário keel para não disputar
sua classificação. Comentários e strings são nós próprios; as palavras
escritas dentro deles não recebem as capturas de keel.

O parser continua sendo o de C. Construções como `buffer i32 xs` e `foreach`
podem gerar nós de erro e prejudicar o reconhecimento estrutural de funções e
declarações ao redor. O realce de C não depende de criar outra gramática;
reconhecimento completo dessas construções exigiria uma gramática de keel.

A extensão instalada pode estar em uma cópia diferente de `editors/zed/`.
No Linux, o link `~/.local/share/zed/extensions/installed/keel` mostra o destino
usado pelo Zed. Atualize essa cópia e execute `zed: reload extensions` na
paleta de comandos. Rodar apenas `gerar.py` atualiza os arquivos deste
repositório.

## O que foi verificado, e o que não foi

**VSCode: medido.** `editors/verifica.mjs` carrega a gramática real do C do
VSCode, aplica a injeção e tokeniza todos os `.k` de `golden/casos/`. O
cabeçalho do arquivo tem como rodar.

    176 ocorrências de palavra keel, 0 sem realce

Verificado também o inverso — nada realçado dentro de comentário ou string — e
que `x.foreach` continua sendo acesso a campo, não palavra.

**Zed: consultas verificadas com Tree-sitter.** O teste
`editors/verifica-zed.py` carrega `tree-sitter-c` v0.24.2, a versão fixada no
manifesto, e compila o `highlights.scm` gerado. Verifica capturas de C e keel no
caso `006-dim`, uma amostra C com diretivas e escapes, a exclusão de palavras
dentro de comentários e strings, e a execução das consultas em todos os `.k`
de `golden/casos/`.

```sh
python3 -m venv /tmp/keel-highlight-venv
/tmp/keel-highlight-venv/bin/pip install tree-sitter==0.25.2 tree-sitter-c==0.24.2
python3 editors/gerar.py
/tmp/keel-highlight-venv/bin/python editors/verifica-zed.py
```

Esse teste verifica capturas, não a cor escolhida pelo tema nem o resultado
visual de uma janela do Zed. Também não promete cobrir cada palavra dentro de
construções que o parser de C não reconhece.

## Nota sobre `.k`

O sufixo colide com a linguagem K em alguns ambientes. Se atrapalhar, o campo a
mudar é `extensions` no `package.json` e `path_suffixes` no `config.toml`.
