module app.lc;
import_c <stdio.h>;

pub int adquire(int i);
pub void solta(int h);

/* Corpo de laço com saídas mistas: `continue`, `break` e `return` vão para
   lugares diferentes, então o cleanup baixa INLINE em cada saída, e não em
   escada (backend §5.5.2). O `[now]` copia no registro. */
pub int roda(int n) {
    int total = 0;
    for (int i = 0; i < n; i++) {
        int h = adquire(i);
        defer [now int h] solta(h);
        if (h == 0) continue;
        if (h < 0)  break;
        if (h > 90) return -1;
        total += h;
    }
    return total;
}
