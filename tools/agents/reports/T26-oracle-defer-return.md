# T26 — relatório do autor de oráculo: `return` com índice sob `defer`, e `break` de `foreach` sob `defer`

## O que foi criado

Arquivos novos, escritos à mão a partir da spec (`keel-c-backend.md` §5.3, §5.5, §5.5.2, §5.7, §6) e dos goldens análogos (005, 015, 017, 032, 037, 039):

- `golden/cases/055-defer-return-index/app/dr2.k` — módulo `app.dr2`, cinco funções.
- `golden/cases/055-defer-return-index/proof.c` — arnês; devolve um código distinto por checagem.
- `golden/cases/055-defer-return-index/expected/c23/app/app_dr2.{type.h,h,c}`
- `golden/cases/055-defer-return-index/expected/c11/app/app_dr2.{type.h,h,c}` — escritos à mão, não derivados do c23; diferem só no comentário da primeira linha, como em 015 e 017.
- Uma linha em `test/cgen/gen/golden-cases.txt`: `055-defer-return-index|app/dr2.k|-|wip`, acrescentada depois da linha de `056-length-fields`, que outro agente tinha deixado não commitada.

Não foram criados `NOTES`, nem `WIP`, nem `VERIFY`. Não se tocou em `golden/line-check.allow`: nenhuma linha do caso precisou de entrada, porque todo código injetado (`{ int keel__rv0 = …;`, o cleanup, `return keel__rv0; }`) cai numa linha do `.k`.

## Escolhas feitas

1. **Formas.** `tail` é a forma do cartão: `defer v[1] = 0; return v[0];`. O cleanup não toca o elemento lido, então ele não mostra a ordem. Por isso acrescentei `head` (`defer v[0] = 0; return v[0];`, esperado 7 e não 0), que mostra a ordem. `pick(size_t i)` cobre o índice variável com `keel_index(i, 3)`, e `two(size_t i)` cobre dois `defer` em ordem inversa (trilha `21`).
2. **`break` de `foreach`.** Usei `buffer i32 *s` como contêiner, espelhando o 015, e não `array`, porque nenhum golden tem `foreach` sobre `array` dentro de função; o §5.7 regra 7 não tem par de referência. O cleanup `count++` sai inline antes do `break` e no fim natural do corpo (§5.5.2, regras 1 e 2), e o `#line 47` aparece duas vezes, cada vez antes de um cleanup.
3. **Forma de saída.** Com uma saída só, o cleanup sai inline, `{ int keel__rv0 = …; <cleanup> return keel__rv0; }` (§5.5.2, regra 1), como no 037 `returned`. Usei `keel__rv0` como nome, pelo precedente de 005 e 037.
4. **Variável de `#line`.** Cada `defer` vira linha em branco no ponto de registro (§5.5, regra 2), e as posições dos `#line` seguem a invariante de §6 (conferidas pelo `line-check`).

## Verificação (`golden/run.sh`, executado pelo agente)

```
ok     055-defer-return-index   c23  (run)
ok     055-defer-return-index   c11  (run)
```

O `proof.c` roda nos dois perfis e devolve `ok`. A única falha do run é outra:

```
FAIL   line-check
         line-check: golden/cases/054-style-forms/expected/c11/app/app_sf.type.h:10: maps to app/sf.k:6, which does not match (not in line-check.allow)
```

Ela está no caso 054, que é de outro agente e não foi tocado aqui. O total do run foi `107 ok, 1 fail, 0 xfail, 0 xpass, 3 wip`.

## Pontos em aberto

- O §5.5 regra 5 mostra `keel__rv1` no exemplo do cleanup inline, mas a regra não diz como N é numerado quando há mais de um `return` com valor. Como este caso tem um `return` por função, `keel__rv0` é o único caminho cuberto. Se a spec quiser que N conte por função, o caso não contradiz, mas não o testa.
- A forma de dois `return` com valor e dois `defer` (escada ou inline) não está coberta, porque o cartão não a pede; isso fica para outro caso.
- O caso não traz `WIP`: o `|wip` em `golden-cases.txt` é o marcador do harness do cgen, e o `run.sh` já o roda e confere.
- Nada foi commitado nem `git add`-ado.
