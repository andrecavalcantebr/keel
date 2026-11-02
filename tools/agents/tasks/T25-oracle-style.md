# T25 — autor de oráculo: constexpr que cita constexpr, `foreach` com `{` na linha seguinte, `walk` com cursor qualificado

Você é o **autor do oráculo** (skill `haiku-task`, seção "Oracle-author cards"). Escreva à mão o caso **`golden/cases/054-style-forms`** (um módulo `app.sf`, arquivo `app/sf.k`, mais `proof.c`, `expected/c23/` e `expected/c11/`). **Não rode `tools/cgen/cgen` sobre o caso nem leia `tools/cgen/src/`**: a saída esperada vem da spec e dos goldens análogos. Quem implementa será outro agente.

## O que o `.k` deve ter (uma função ou constante por forma)

1. **`constexpr` que cita `constexpr` do módulo** (hoje o emissor deixa o nome sem prefixo e o C não compila): `pub constexpr size_t A = 2;`, `pub constexpr size_t B = A * A;`, `pub constexpr size_t C = B + A;`, e uma `pub size_t c(void) { return C; }`. Os nomes dentro do `INIT` são renomeados como em qualquer uso (`app_sf_A`). C23: `constexpr size_t app_sf_B = app_sf_A * app_sf_A;`. C11 (backend §9.2): `#define app_sf_B ((size_t)(app_sf_A * app_sf_A))` e o objeto de conferência `static const size_t app_sf_B__chk = (app_sf_A * app_sf_A); _Static_assert(…)`. Modelo: `golden/cases/036-constexpr-lines` e `045-extern-forms`.
2. **Estilo Allman** (chave na linha seguinte), na função e no `foreach`:
   ```
   pub i32 allman(slice.slice i32 s)
   {
       i32 t = 0;
       foreach (i32 x, size_t i : s)
       {
           t += x - (i32)i;
       }
       return t;
   }
   ```
   O C conserva as linhas do autor (cada linha do `.k` tem a sua no `.c`, §6): a linha do `foreach` vira `{ keel_slice_i32 keel__c0 = s; size_t keel__n0 = keel_slice_i32_length(keel__c0); for (size_t i = 0; i < keel__n0; i++)` (sem chave), a linha `{` seguinte vira `{ i32 x = keel_slice_i32_get(keel__c0, i);`, e a `}` do corpo vira `} }`. Backend §5.7 (regras 1 a 5) e o `foreach` de `golden/cases/015-foreach-range`/`031-readme-coop`.
3. **`walk` com o cursor escrito com o nome do próprio módulo, qualificado** (módulo de dois segmentos): `walk (i32 *p, app.sf.cursor c : l)` sobre uma lista ligada que o módulo declara, com `begin`/`has_next`/`next`. O tipo qualificado é renomeado (`app_sf_cursor c`). Modelo: `golden/cases/019-walk` (use a mesma lista e o mesmo `walk`, mudando só a escrita do cursor).

## Arquivos a criar

`golden/cases/054-style-forms/app/sf.k`; `golden/cases/054-style-forms/proof.c` (roda e confere os valores de cada forma); `expected/c23/app/app_sf.{type.h,h,c}` e o mesmo em `expected/c11/`; as instâncias `keel/…` que o módulo gera ficam **fora** do `expected` quando já estão em `golden/c23/keel/` e `golden/c11/keel/` (veja como `004-parallel` e `025-of-range` fazem). Acrescente a linha `054-style-forms|app/sf.k|-|wip` a `test/cgen/gen/golden-cases.txt`. Se o `golden/line-check.py` pedir, acrescente a `golden/line-check.allow` **só** linhas de código injetado (use `python3 golden/line-check.py --allow | grep 054-style-forms`).

Não toque em mais nada. Não faça commit nem `git add`.

## Aceitação

`golden/run.sh` termina sem `FAIL` com o caso `054` em `ok` nos dois perfis (compila, roda o `proof.c` e passa `layout-check` e `line-check`). Se a spec for ambígua em algum ponto, escolha o que os goldens análogos fazem e **liste a escolha** no relatório (`tools/agents/reports/T25-oracle-style.md`, pt-BR). Entregue o relatório UMA vez pelo SubagentHandback e termine.
