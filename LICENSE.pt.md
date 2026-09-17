# keel — Licença (tradução informal)

**Este arquivo não tem valor jurídico.** O único texto com efeito legal é o inglês, em
[`LICENSE.md`](LICENSE.md). A razão é a mesma que a FSF dá para toda tradução da GPL, e vale
citá-la por extenso, porque é exatamente o motivo de este arquivo existir do jeito que existe:

> A FSF não aprova traduções de licença como oficialmente válidas. O motivo é que
> verificá-las seria difícil e caro (exigiria a ajuda de advogados bilíngues em outros
> países). Pior ainda: se um erro passasse despercebido, o resultado poderia ser desastroso
> para toda a comunidade de software livre. Enquanto as traduções são não-oficiais, elas não
> podem causar dano jurídico nenhum.
> — <https://www.gnu.org/licenses/translations.html>, tradução livre

## A GPLv3 em si

Este projeto não traduz o corpo da GPLv3 — não por preguiça, mas porque retraduzir um texto
jurídico de várias centenas de linhas é exatamente o tipo de trabalho que deveria passar por
revisão jurídica bilíngue antes de circular, e fazer isso por conta própria correria o risco
que o parágrafo acima descreve. Quem quiser ler a GPLv3 em português encontra uma tradução
não-oficial, mantida por tradutores da comunidade e listada pela própria FSF, em:

- <http://licencas.gitlab.io/gpl-3.0.pt-br.html>
- índice completo de traduções da FSF: <https://www.gnu.org/licenses/translations.html>

O texto que governa este repositório continua sendo o original em inglês.

## A exceção da Base keel (esta, sim, é nossa — a tradução é segura)

O texto abaixo é a nossa própria redação, então traduzi-lo carrega bem menos risco que
retraduzir a GPL. Ainda assim, em caso de qualquer divergência entre esta tradução e o texto
em inglês de `LICENSE.md`, **o inglês prevalece**.

---

### Exceção da Biblioteca Base keel, versão 1

Permissão adicional sob a seção 7 da GPLv3.

Este é um aviso legal, aplicável aos arquivos que compõem a Biblioteca Base keel — os
fontes `.k` sob `src/base/`, e qualquer arquivo que leve este aviso —, chamada abaixo de "a
Base".

**O que fica isento.** Quando o `cgen` (o transpilador keel) processa um fonte `.k` ou `.c`
seu — que não seja, ele mesmo, parte da Base — e produz mecanicamente fonte C ou headers a
partir dele, esse resultado gerado não se torna um trabalho baseado na Base só por conter,
textualmente, fragmentos copiados ou derivados dela. Você pode distribuir esse resultado
gerado, e qualquer programa construído a partir dele, sob os termos de licença que escolher.

**O que continua coberto.** Esta exceção não vale se você distribuir uma cópia da própria
Base, ou uma versão modificada dela, separadamente ou embutida em outro produto: essa
distribuição segue a GPLv3 comum, sem exceção, como qualquer outro trabalho coberto por ela.
Ela também não se estende ao `cgen`: uma versão modificada do `cgen` que você distribua segue
a GPLv3 comum, sem exceção. Esta exceção trata só do que acontece com a saída que o *seu*
fonte produz ao passar por uma Base não modificada — não da licença de uma Base modificada ou
de um `cgen` modificado.

Se você quiser redistribuir a própria Base sob outros termos, ou tiver dúvida se o seu uso
está coberto, entre em contato com o titular dos direitos autorais.

Copyright (C) 2026 Faculdade de Engenharia Elétrica e de Computação, Universidade
Federal do Amazonas (FEEC/UFAM) *(titular a confirmar — ver nota)*.
Todos têm permissão para copiar e distribuir cópias fiéis deste aviso de exceção, mas
alterá-lo não é permitido.

---

> **Nota de rascunho.** Ver a mesma nota em `LICENSE.md`: o titular do direito autoral e a
> redação final devem passar pela Pró-Reitoria de Inovação Tecnológica da universidade antes
> de qualquer um dos dois arquivos valer como licença definitiva.
