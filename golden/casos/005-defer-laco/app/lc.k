module app.lc;

priv int vivos  = 0;
priv int soltos = 0;

priv int  adquire(int i) { vivos++; return i == 3 ? 0 : (i == 7 ? 95 : i + 1); }
priv void solta(int h)   { (void)h; vivos--; soltos++; }

pub int  vivos_agora(void)  { return vivos; }
pub int  soltos_agora(void) { return soltos; }
pub void zera(void)         { vivos = 0; soltos = 0; }

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
