---
name: keel-libs-reorganizacao-2026-11-20
description: keel_libs virou um repositório só (coll, maths); a stdlib é a base e mais nada; libs-spec.md; distribuição só em fonte; LIBS.md e CONTRIBUTING
metadata:
  type: project
---

Em 2026-11-20, `../keel_libs` virou **um repositório git** (master, sem remoto ainda) com uma pasta por biblioteca: `coll/` (ex-keel_coll) e `maths/` (ex-keel_maths); os módulos continuam `coll.*` e `maths.*`. Antes não havia histórico a preservar (keel_coll sem commits; keel_maths com 1 baseline + 57 arquivos sujos). Backup dos dois `.git` em `~/code/cprojects/keel_libs-backup-2026-11-20.tar.gz`. `Papers/` (PDFs de terceiros) fica no .gitignore.

Decisões do André (mesma data):
- **A stdlib é a base (`base/`) e mais nada.** Todo o resto é biblioteca: mantida (pasta em keel_libs, entra por PR se cumprir os critérios) ou de terceiros (repositório do autor, listada em `keel_libs/LIBS.md`). Resolve a D11; ROADMAP §3 e spec §5.9 já valiam.
- **Sem prefixo `keel.`** nas bibliotecas (`keel` e `keel.<nome>` são da base, ferramenta §8). Nome do repositório: `keel_libs` ("keel_std" foi descartado: dá a ideia de padrão).
- **Distribuição é só em fonte** (`-I` + `import`): genéricos e instâncias só existem a partir do `.k`. Binário (.a) tem dois usos: biblioteca C dentro de keel (`import_c` + `-l`) e exportar de keel para C (`.h` + `.a` gerados). Manual §7.6.
- `file`/`net`/`time`: **wrapper fino de C** com a convenção da biblioteca (arena, outcome, slice/buffer, walk), não buffer/ponteiro cru. **HTTP/HTTPS: nada escrito; se entrar, é biblioteca separada** (parser sem E/S primeiro; TLS é dependência externa). Discutir antes.
- `tensor`: ainda sem definição; vai ser feita (posto como `dim`, strides por eixo; mview já prova a base).
- Pendente: licença de keel_libs como um todo; se a dist do cgen traz cópia das libs; criar remoto no GitHub (monorepo, não submódulos).

O documento é `keel/design/libs-spec.md` (era stdlib-spec.md): **normativo para as bibliotecas, não para a keel**; contrato comum, catálogo de coll/maths, candidatos (buffer(N), string/strview, grid, slice.from, atomic/chan/barrier, file/path/net/time), decisões D1-D13. A §1 de possibilidades.md passou a apontar para ele.

**Why:** o André quer a linguagem fechada e as bibliotecas como carga de prova; alunos já usam a keel e precisam de um lugar para contribuir.
**How to apply:** não chamar nada de "stdlib" fora da base; achados de biblioteca voltam como bug report; ver [[stdlib-e-keel-libs-independentes]].
