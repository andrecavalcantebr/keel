# T32 — autor de oráculo: `walk` com a `{` do corpo em linha própria

Você é o **autor do oráculo** (skill `haiku-task`, seção "Oracle-author cards"). Escreva à mão o caso **`golden/cases/057-walk-allman`** (módulo `app.wa`, `app/wa.k`, `proof.c`, `expected/c23/`, `expected/c11/`). **Não rode `tools/cgen/cgen` sobre o caso nem leia `tools/cgen/src/`**; não rode `make` nem `verify.sh`.

## A forma (hoje o cgen gera C que não compila)

Uma lista ligada percorrida por `walk`, com a chave do corpo na linha seguinte (estilo Allman), na função e no `walk`:

```
pub i32 sum(List *l)
{
    i32 t = 0;
    walk (i32 *p, app.wa.cursor c : l)
    {
        t += *p;
    }
    return t;
}
```

Use a mesma lista, os mesmos verbos `begin`/`has_next`/`next` e o mesmo cursor do caso **`golden/cases/054-style-forms`** (leia `app/sf.k` e `expected/c23/app/app_sf.c`: o `walk` com chave na mesma linha e o `foreach` Allman). A regra é a do backend §5.7 regra 5 (texto novo): o cabeçalho sai na sua linha **sem a chave do corpo**, a linha da `{` abre o corpo e leva a declaração do binder (`{ i32 *p = app_wa_next(keel__c0, &c);`), e a `}` do corpo fecha também o bloco do `walk` (`} }`); cada linha do fonte continua com a sua no C. O `walk` de chave na mesma linha de 054 mostra o cabeçalho (`{   app_sf_List *keel__c0 = l; app_sf_cursor c = app_sf_begin(keel__c0); while (app_sf_has_next(keel__c0, &c)) { i32 *p = app_sf_next(keel__c0, &c);`): aqui o `while (…)` termina a linha do `walk` e o `{ i32 *p = …;` vai na linha seguinte.

## Arquivos e aceitação

`golden/cases/057-walk-allman/…`; a linha `057-walk-allman|app/wa.k|-|wip` em `test/cgen/gen/golden-cases.txt`; `golden/line-check.allow` só para código injetado (`python3 golden/line-check.py --allow | grep 057`); `proof.c` que roda e confere o valor; `golden/run.sh` sem `FAIL`. Não toque em mais nada, nem faça commit. Não nomeie um símbolo local igual a um do módulo (a spec §2.5 diagnostica sombreamento). Relatório em `tools/agents/reports/T32-oracle-walk-allman.md` (pt-BR), com as escolhas feitas onde a spec for ambígua. Entregue UMA vez pelo SubagentHandback e termine.
