# golden — casos normativos

Cada caso é um par: o fonte keel (`caso.k`) e **o C que ele deve produzir**
(`esperado.c`, `esperado.h`), escrito à mão a partir da spec.

Servem a duas coisas, e a segunda foi a que os motivou:

1. **Fixtures do cgen.** Quando o parser existir, `cgen caso.k` tem de produzir
   `esperado.c` — não byte a byte necessariamente, mas com o mesmo sentido, que
   é o que a spec §7.1 obriga.
2. **Verificação da própria spec.** O C esperado **compila**, nos dois perfis,
   com `-pedantic-errors`. Onde não dá para escrevê-lo, é lacuna da spec — e foi
   assim que o caso 002 nasceu.

## Como roda

    ./run.sh            # gcc; CC=clang ./run.sh para outro compilador

Perfil C23 compila com `-std=c2x` sobre `c23/`; perfil C11 com `-std=c11` sobre
`c11/`. Um caso sem `esperado.c11.c` usa o mesmo arquivo nos dois.

## Os dois perfis são árvores separadas, e isso é normativo

`cgen` emite **um** perfil, já resolvido — o gerado nunca tem `#if` de versão.
Confirmado na prática: o GCC 13 reporta `__STDC_VERSION__ == 202000L` sob
`-std=c2x`, não `202311L`, então detectar C23 pelo pré-processador não funciona.
É a razão de `cgen-tool-spec.md §4.9` ler o perfil da linha de comando.

Os headers fixos (`keel/prelude.h`, `keel/arena.h`) e os de instância vivem em
`c23/keel/` e `c11/keel/`, e diferem exatamente no que `backend §9.1` lista.

## Casos com PROBLEMA

Um caso que contém um arquivo `PROBLEMA` é **xfail**: espera-se que o C esperado
*não* compile, e o arquivo registra a lacuna da spec e a saída candidata. Se um
xfail passar a compilar, o runner reporta `XPASS` — o problema foi resolvido e o
arquivo deve sair.
