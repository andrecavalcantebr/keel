# keel — distribuição

Esta pasta é a instalação inteira: uma árvore relocável. Descompacte em qualquer
lugar e ponha `bin/cgen` no `PATH` (`./install.sh` faz um symlink em
`~/.local/bin`; `./uninstall.sh` o remove). O `cgen` acha a base em
`lib/base` a partir do próprio executável, sem opção nem variável de ambiente.

```plain
bin/cgen        o transpilador e driver: transpila e chama o compilador C
lib/base/       os módulos keel.*: o prelúdio keel.k e keel/*.k
lib/src/        o fonte do cgen, com a base de apoio dele já gerada (./build.sh)
doc/            a linguagem, o rationale, o backend C e a ferramenta
editors/        destaque de sintaxe (VS Code, Zed)
```

## Um programa

```keel
module ola;
import_c <stdio.h>;

pub int main(void) {
    puts("Ola, keel!");
    return 0;
}
```

```sh
cgen ola.k -o ola && ./ola
```

`cgen` é um driver no molde do `gcc`: o que não é opção própria dele atravessa
para o compilador C (`-O2`, `-std=c23`, `-fopenmp`, `-I`). O C gerado fica em
`./gen` (`--dest-dir`). Para só gerar o C, `--stop-after=gen`. A ajuda completa
é `cgen --help`.

## Reconstruir o cgen

```sh
cd lib/src && ./build.sh      # escreve ./cgen
```

## Licenças

O cgen e a base seguem `doc/LICENSE.md` (GPLv3, com a exceção que isenta o código
que a base contribui ao C gerado do usuário); os documentos, `doc/LICENSE-DOCS.md`.
Há versões em português (`*.pt.md`).
