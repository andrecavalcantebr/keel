# E1 — corrigir o exemplo do backend §5.10 (autorizado pelo André)

## Objetivo
Trocar o exemplo e a regra 1 da §5.10 de `keel-c-backend.md` pelo texto
aprovado abaixo. É a única edição normativa autorizada.

## Arquivos
Editar somente `keel-c-backend.md`, e somente a §5.10 (linhas ~1412-1450,
de `### 5.10` até `### 5.11`). Criar `tools/agents/reports/E1.md`.

## Contrato
1. No bloco ```c do exemplo, substituir tudo por:

```c
//C gerado
typedef keel_corot (*keel_routine_app_Ctx)(app_Ctx *);

typedef struct keel_routine_slot_app_Ctx {
    keel_routine_app_Ctx f;
    app_Ctx             *ctx;
    keel_corot           state;
} keel_routine_slot_app_Ctx;

keel_routine_slot_app_Ctx steps[2] = {
    { .f = app_prepare, .ctx = &ctx, .state = {0} },
    { .f = app_measure, .ctx = &ctx, .state = {0} },
};
keel_outcome_u32 r =
    keel_routine_app_Ctx_par(keel_slice_of_keel_array_keel_routine_slot_app_Ctx(steps, 2), 1);
```

(mantenha a linha `//C gerado` e a cerca exatamente como o bloco atual as
tem; `.state = {0}` fica explícito de propósito, decisão do André.)

2. No bloco ```keel, corrigir só o alinhamento: as duas linhas do
   inicializador ficam `{ .f = prepare, .ctx = &ctx },` e
   `{ .f = measure, .ctx = &ctx },` (um espaço, sem alinhamento extra).
3. Regra 1: reescrever para
   "O `typedef` da participante é da instância e sai no header de instância
   (§4.3), com o nome do módulo instanciado: `keel_routine_<mod>_<arg>`, onde
   `<mod>` é o prefixo do módulo do argumento (`keel_routine_app_Ctx` para
   `app_Ctx`). O `slot` e os verbos seguem o mesmo nome
   (`keel_routine_slot_app_Ctx`, `keel_routine_app_Ctx_par`). O encurtamento
   do §2.1 (regra 2) se aplica."
4. Regra 2: acrescentar ao fim "O inicializador escreve o `.state = {0}`
   explicitamente; um mecanismo de inicialização explícita de modificadores
   virá depois."
5. Não alterar mais nada: regras 3 e 4, "Verificações", outras seções.

## Passos sugeridos
Ler a seção, editar com o Edit tool, reler.

## Aceitação
- `git diff --stat` mostra só `keel-c-backend.md` (e o relatório, que é
  não rastreado).
- `git diff -U0 keel-c-backend.md` só tem hunks entre as linhas da §5.10.
- `grep -n "keel_routine_par_Ctx\|slot_Ctx_from" keel-c-backend.md` não
  imprime nada.

## Relatório
`tools/agents/reports/E1.md`: o diff aplicado e a saída dos três comandos.
