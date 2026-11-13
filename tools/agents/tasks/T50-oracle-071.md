# T50 — oráculo do caso 071: função sobre protocolo, tipo concreto de outro módulo

## Objetivo

Escrever **à mão** o caso `golden/cases/071-protocol-function-instance/`: uma
função sobre protocolo (spec §4.14) declarada num módulo, chamada com um tipo
concreto que é **instância de um modificador de outro módulo genérico**. Nenhum
golden exercita isso (065: tipo comum do mesmo módulo; `keel.slice.of`: só via
`--instance`). Você é o **autor do oráculo**: não implementa nada.

## Proibido (captura da ferramenta)

- **Não rode `tools/cgen/cgen` neste caso**, nem leia `tools/cgen/src/`. O
  esperado sai da spec e do backend, por raciocínio, e dos goldens análogos.
- Não edite normativos, código, nem outros casos. Não commite.

## Arquivos (só estes)

Cria `golden/cases/071-protocol-function-instance/` com:

- `app/ibox.k` e `app/ibox_use.k` (fontes abaixo, **exatamente assim**)
- `proof.c`, `NOTES`
- `expected/c23/` e `expected/c11/`, **escritos separadamente** (nunca um
  derivado do outro por cópia mecânica: confira cada diferença de perfil no
  backend §2/§3, como `[[nodiscard]]` no c23)

Edita: `test/cgen/gen/golden-cases.txt` (linha nova, com `|wip`, formato das
linhas vizinhas: `071-protocol-function-instance|app/ibox_use.k|-|wip`),
`golden/line-check.allow` (só se houver código injetado). Cria
`tools/agents/reports/T50-oracle-071.md`.

## Fontes

```keel
// app/ibox.k
module app.ibox type T;

pub modifier box { T *values; size_t length; }

pub inline size_t length(box *b) { return b->length; }
pub inline child T *ptr(parent box *b, size_t i) { return &b->values[i]; }
```

```keel
// app/ibox_use.k
module app.ibox_use;
import app.ibox types;

pub inline size_t count(IndexPtr items) { return length(items); }

pub size_t probe_count(void) {
    i32 arr[3] = {10, 20, 30};
    box i32 b = { arr, 3 };
    return count(b);
}
```

`box` não é `byref`: o argumento vai **por valor** (spec §4.14, regra 7), e o
verbo `length` do protocolo recebe `C *x`, então o corpo da instância chama o
verbo do tipo concreto com `&items` (como o `pf_limit(&items)` do 065).

## O que o esperado deve afirmar

1. A instância `count` sobre `app.ibox.box i32` tem o seu par de headers próprio
   (backend §5.19, regras 1 e 4), nomeado pelo prefixo do módulo da função e
   pelo nome canônico do tipo concreto (`app_ibox_box_i32`); o `.h` inclui o
   `.type.h` do tipo concreto e o `.h` do tipo concreto, **cujo verbo o corpo
   chama** (`app_ibox_box_i32_length`).
2. O corpo reescreve `length(items)` para o verbo do tipo concreto, com a
   adaptação da §4.4 (`&items`).
3. `app_ibox_use.c` inclui o header da instância e a chamada vira
   `app_ibox_use_count_app_ibox_box_i32(b)`.
4. O módulo genérico `app.ibox` sai como nos goldens com `box`: veja
   `golden/cases/067-generic-const-view/expected/` (`app_cbox*.h`,
   `app_cbox_box_i32*.h`) e copie o **raciocínio**, não os arquivos.
5. Cada arquivo abre com o cabeçalho de uma linha do cgen
   (`/* <arq> — generated from <fonte>.k by cgen, C23 profile. */`), e as
   diretivas `#line` seguem backend §6.

## Análogos a ler

- `golden/cases/065-protocol-function/` (tudo): função sobre protocolo, tipo
  comum. É o molde dos headers da instância.
- `golden/cases/067-generic-const-view/` (tudo): módulo genérico com
  `modifier box` e o `.c` do módulo de uso.
- `keel-spec.md` §4.14; `keel-c-backend.md` §5.19, §4.3, §6, §2.1.1.
- `golden/README.md` e `golden/NOTES` (o formato do `proof.c` e do `NOTES`).

## `proof.c`

Inclui `app/app_ibox_use.h` e retorna 1 se `app_ibox_use_probe_count() != 3`,
senão 0.

## Aceitação

```sh
golden/run.sh 2>&1 | tail -15
```

O caso 071 deve compilar, executar e passar nos checks de layout e de `#line`,
nos dois perfis. `test/cgen/gen` não precisa passar neste caso (`|wip`).

## Relatório

`tools/agents/reports/T50-oracle-071.md`, em pt-BR: arquivos criados, a saída
do `golden/run.sh` para o 071, e **cada dúvida** em que a spec/backend não
decide o texto do esperado (não adivinhe; liste).
