# limites-auditoria — levantar os limites de implementação do cgen

## Objetivo

Produzir um relatório com **todos os limites de implementação que um usuário
pode atingir** ao usar a keel com o cgen: número máximo de linhas, bytes por
linha, arquivos, itens por construção, profundidades, tamanho de nomes etc. O
relatório vira a tabela de limites do manual e do `keel-llm-context.md`; quem a
escreve é o orquestrador. É uma auditoria de **leitura**: não altere código.

## Arquivos

- Cria: `tools/agents/reports/limites-auditoria.md`. Nada mais. Não edite
  código, documentos normativos, golden nem testes; não faça commit.

## Contrato

Para cada limite, o relatório diz: **nome** (a constante, ou a regra),
**valor**, **o que limita** (em termos do usuário: "itens de um `tags`", "bytes
de C por linha física do fonte"), **configurável?** (por `-D` na compilação do
cgen, variável de ambiente ou fixo), **diagnóstico** quando atingido (id e
mensagem, se houver; senão "recusa pelo `cc`" ou "silencioso"), e **onde**
(`arquivo:linha`).

Onde procurar, no mínimo:

- `tools/cgen/src/tool/cli_limits.h`, `tool.h`, `main.c`, `memory.h`,
  `stop_gen.c` (`WRITTEN_CAP`, `USE_CAP`), `stop_parse.c`.
- `tools/cgen/src/engine/storage_types.h` (`K_INSTANCE_NAME_CAP`, `K_SYMBOL_MAX`,
  `K_DETAIL_MAX` e vizinhos), `diag.h`.
- `tools/cgen/src/engine/emit/*.c` e `emit_internal.h`: toda constante `*_CAP`,
  `MAX_*` e `#define` de tamanho (`LINE_CAP`, `MAX_EDITS`, `SCRATCH_CAP`,
  `MAX_RANK`, `MAX_COLUMNS`, `MAX_GROUPS`, `MAX_FIELDS`, `MAX_LABELS`...).
- `tools/cgen/src/engine/parser/*.c|h`: limites de binders, de delimitadores,
  de `tags`, de profundidade de instância (`instance-depth`), de aninhamento.
- Os textos de `implementation-limit` (`grep -rn "implementation-limit"`) e os
  ids `tags-limit-exceeded`, `instance-depth`, `bitbuffer-width-invalid`.
- Os normativos (`keel-spec.md`, `keel-c-backend.md`, `cgen-tool-spec.md`): o que
  já dizem de limite (`grep -n -i "limite"`), p. ex. nome de 255 caracteres
  (backend §2.4), `mask` de 64 slots, `tags` de 128.
- `test/cgen/` e `golden/cases/081-large-table`: testes que exercitam limites.

Separe duas tabelas: **A. limites do usuário** (atingíveis por um programa
keel razoável) e **B. limites internos** (só atingíveis por entrada absurda ou
gerada: capacidades de buffer do emissor). Em caso de dúvida, B.

**Meça, não leia comentário.** Para os de A com valor numérico, confirme ao menos
um por um programa `.k` mínimo gerado em `/tmp/` (por exemplo, um `tags` de 129
itens deve dar `tags-limit-exceeded`) rodando `tools/cgen/cgen --stop-after=gen`.
Registre o comando e a saída. Se um limite lido no código não se comportar como
a leitura sugere, diga isso, sem corrigir.

## Passos sugeridos

1. `grep -rnE "define +[A-Z_]*(CAP|MAX|LIMIT)[A-Z_]*" tools/cgen/src`.
2. Para cada resultado, leia o uso e decida A ou B.
3. Confirme os de A por programa mínimo em `/tmp/`.
4. Escreva o relatório.

## Aceitação

- `git status --short` lista só `tools/agents/reports/limites-auditoria.md`
  como novo (mais o que já estava sujo antes: `gdb/__pycache__/`, `gen/`,
  `skills/`).
- O relatório tem as tabelas A e B, cada linha com `arquivo:linha`, e uma seção
  "Dúvidas" (pode ser vazia).

## Relatório

É o próprio arquivo acima, em pt-BR (código e ids em inglês), com a seção
"Dúvidas": tudo em que o código, os documentos e o comportamento medido
discordam.
