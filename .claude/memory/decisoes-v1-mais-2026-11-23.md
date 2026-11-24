---
name: decisoes-v1-mais-2026-11-23
description: "Decisões do André (2026-11-23) sobre pub import/pacote, intervalos, teto de nome 215, protocolo nominal, handle, limites; o que está decidido e o que espera \"sim\""
metadata:
  node_type: memory
  type: project
  originSessionId: a30bc17f-45e1-4b2d-8fa8-c5040b5a7d64
  modified: 2026-11-23T22:47:21.653Z
---

Decididas em conversa (2026-11-23). Cada texto normativo passa por "proponha o texto, espere o sim" ([[discutir-antes-de-editar-spec]]). Estado da sessão e pendências em [[sessao-2026-11-23-estado]].

**Já aplicado (com "sim" do André):**
- **Teto de nome gerado = 215** (era 255; backend §2.4 regra 1 + regra 5, D21, rationale "Por que o teto de nome é 215", *desde 0.4.0*). Razão: o nome do símbolo da instância é também o nome do arquivo; ext4/NTFS/APFS aceitam 255 por componente, e há sufixos (`.type.h` 7, temporário, `.gch`, `.o`, `.d`), então 215 deixa 40 de folga. Windows: caminho inteiro 260 por padrão (salvo `LongPathsEnabled`), nota no manual. Implementação: `K_GENERATED_NAME_MAX` 215 em `storage_types.h`, `NAME_CAP` 1024, temporário `.keel.<pid>.tmp`; testes `cli/name_limit.sh`, `cli/imports.py`. Fecha o estudo E01 (opção A).
- **`LINE_CAP`** (1024 bytes de C por linha física) está no cgen-tool-spec §7.1 (linha do `implementation-limit`) e na tabela de limites do manual §7.8 e do keel-llm-context.
- **E03 extent**: >4 grupos é `extent-limit-exceeded` (só em `diag-messages.md`).

**Decididas, texto ainda por propor/aprovar:**
- **Intervalos** (nada escrito nos normativos ainda; o André disse "sim" ao 215, não confirmou o texto dos intervalos): `a..b` ≡ `a..<b` (meio-aberto); `a..=b` fechado = `[a, b+1)`; sem limite superior, `a..`, `a..<` e `a..=` são o mesmo (vão até o fim, incluindo o último); sem `a`, início zero. Overflow: `b == SIZE_MAX` aborta em debug (`inclusive-range-overflow`), em release satura (`b+1` fica `SIZE_MAX`; ninguém indexa `v[SIZE_MAX]`). Emissão: `keel_inclusive(b)` função em linha do prelúdio (como `keel_index`; `KEEL_CHECK` por dentro, ligado por `KEEL_CHECKS`). **Sem texto de depreciação do `a..b` por ora** (até a 1.0.0 se decide). Marca *(desde 0.4.0)*. Golden: caso novo (foreach, `x[a..=b]`, literal, abort em debug), esperado à mão, entrada em `gen/golden-cases.txt` com `|wip` até implementar.
- **`pub import` por módulo** (não por símbolo; como `import java.util.*`): `pub import tensor.operations as ops` dentro de `module tensor` dá a quem faz `import tensor` o alias `tensor.ops`; só alias, o módulo dono e o mangling não mudam. **`source` descartado** (seria `#include` da keel). Pacote = fachada + pasta, por ora convenção. **Busca (b)**: `-I` só para o módulo fachada (`-I ./lib/tensor`); os demais módulos do pacote são achados de forma **relativa** à fachada, para a estrutura interna poder mudar sem o usuário notar. Esperar o `pub import` sair antes de decidir mais sobre pacotes.
- **Protocolo nominal**: fica no ROADMAP v2+ (`foreach` sobre `T` opaco dentro de genérico; função sobre protocolo já cobre).
- **Handle**: discutir depois dos ajustes; preferência por DOD: índice geracional sobre `pool` (`entity {index, gen}`, 8 bytes, sem ponteiro), `free(invalidates e)`.
- **Comparador como eixo (B3/B6)**: só depois.
- **Arena (64 MiB)**: pico medido em `keel_libs/coll` = 12,8 MiB, golden 7,5 MiB. O André acha que vai ser preciso aumentar (vários tetos nunca disparam porque a arena acaba antes: ~400 módulos, ~10 000 funções): decidir entre subir a arena ou baixar os tetos.

**Why:** o André quer fechar organização de bibliotecas (maths/tensor) e limites antes da 1.0.0.
**How to apply:** partir daqui, não reabrir; propor texto normativo antes de escrever.
