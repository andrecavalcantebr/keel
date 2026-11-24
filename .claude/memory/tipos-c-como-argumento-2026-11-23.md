---
name: tipos-c-como-argumento-2026-11-23
description: "Decisão de 2026-11-23: palavras C aritméticas (unsigned char, long long) são argumento de modificador, normalizam para o tipo keel e avisam (c-type-spelling); long fica recusado"
metadata:
  type: project
---

Spec §4.2 regra 3 + `base-type`, backend §2.1, rationale ("Modificador e tipo modificado"), manual §3.3, golden 086 (commitado na 0.4.0, sem push).

- Tabela: signed/unsigned char→i8/u8, short→i16/u16, int/signed/unsigned→i32/u32, long long→i64/u64, float/double→f32/f64, qualquer ordem. `char`/`bool` ficam. `long`, `unsigned long`, `long double` seguem `c-type-as-argument` (largura do alvo).
- `c-type-spelling` (warning, desde 0.4.0) diz o nome idiomático; `-Wno-c-type-spelling` cala.
- O C emitido usa o nome keel (`long long` e `int64_t` são tipos distintos em LP64) e a etiqueta renomeada do struct (`struct recs_Person`).
- Só vale em argumento de modificador; `unsigned char x;` solto segue C opaco.

**Why:** o André quer que código C em migração compile e que a saída da compilação ensine o idioma (u8), e o rationale já previa `base-type` ampliado.
**How to apply:** um `typedef` de instância (`typedef buffer u8 text;`) também vale agora como a instância (A2, golden 085); A1 e A3 de keel_libs fechados (083, 084). Ver [[sessao-2026-11-23-estado]].
