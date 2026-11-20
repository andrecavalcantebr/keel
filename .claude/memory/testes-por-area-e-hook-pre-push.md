---
name: testes-por-area-e-hook-pre-push
description: test/run.sh aceita áreas (unit cli lex parse diag gen golden golden:<caso> docs); a suíte inteira só antes do push, disparada por .git/hooks/pre-push
metadata:
  type: feedback
---

A suíte inteira passou de 10 min e o André a achou tediosa. Regra (2026-11-20, e em AGENTS.md): durante o trabalho, `test/run.sh AREA...` roda só a fatia afetada; `golden/run.sh <palavra>` filtra casos; a suíte completa (`test/run.sh` sem argumento) roda **antes de cada push**, e o hook local `.git/hooks/pre-push` a dispara (fora do git; `git push --no-verify` pula). Tempos: unit ~150s, cli ~55s, gen ~45s, lex 13s, parse 10s, diag 3s, docs 0s. O cabeçalho do script diz o que rodar depois de mexer em quê (parser: lex parse diag gen; emissor: gen golden:<caso>; driver: unit cli).

**Why:** o ciclo de commit local ficava lento demais; o risco de regressão distante é coberto no push.
**How to apply:** não rodar a suíte inteira a cada commit; escolher as áreas; avisar quando só uma fatia rodou. O push demora ~10 min por causa do hook.
