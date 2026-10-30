# E4 — regra 1 da §5.10, exemplo do README e nome do buffer de slice (autorizado pelo André)

## Objetivo
Três correções de texto, aprovadas pelo André.

## Arquivos
Editar somente `keel-c-backend.md` (linhas 113 e 896, e a regra 1 da §5.10)
e `README.md` (exemplo de `routine`, linhas ~527-580). Criar
`tools/agents/reports/E4.md`.

## Contrato
1. `keel-c-backend.md`, regra 1 da §5.10: substituir o texto da regra 1 por
   exatamente:
   "1. O `typedef` da participante é da instância e sai no header de instância (§4.3), com o nome do módulo mais o nome canônico do argumento (§2.1): `keel_routine_<arg>`, como em `keel_routine_app_Ctx` para `app_Ctx` e `keel_routine_i16` para `i16`. O `slot` e os verbos seguem o mesmo nome (`keel_routine_slot_app_Ctx`, `keel_routine_app_Ctx_par`). O encurtamento do §2.1 (regra 2) se aplica."
2. `keel-c-backend.md:113`: `keel_buffer_slice_char` → `keel_buffer_keel_slice_char`
   (mantenha o alinhamento da seta `→`; as linhas vizinhas não mudam).
3. `keel-c-backend.md:896`: `keel_buffer_slice_char_ptr` →
   `keel_buffer_keel_slice_char_ptr`.
4. `README.md`, exemplo de `routine` (procure `keel_routine_par_i16`):
   - `keel_routine_par_i16` → `keel_routine_i16_par` (definição e chamada);
   - `uint32_t` → `u32` dentro desse exemplo (como o golden:
     `golden/cases/010-routine/expected/c23/keel/keel_routine_ag2_Ag.h:43`);
   - inicializadores `{ coop_stage, a, {0} }` → `{ .f = coop_stage, .ctx = a, .state = {0} }`
     (idem `b`);
   - `keel_slice_keel_routine_slot_i16 s = { 2, steps };` + chamada com `s` →
     tirar `s` e chamar
     `keel_routine_i16_par(keel_slice_of_keel_array_keel_routine_slot_i16(steps, 2), 1)`;
   - `keel_routine_slot_i16` fica como está.
   Se o exemplo usar `uint32_t` por razão que o contexto justifique (ex. um
   trecho de libc), não troque e relate. Não mexa em mais nada do README.
5. Nenhuma outra linha de nenhum arquivo muda.

## Aceitação
- `git diff --stat`: só `keel-c-backend.md` e `README.md`.
- `grep -n "keel_routine_par_\|buffer_slice_char" keel-c-backend.md README.md`
  não imprime nada.
- `git diff -U0` mostra só as mudanças dos itens 1–4.

## Relatório
`tools/agents/reports/E4.md`: o diff e a saída dos comandos; dúvidas.
