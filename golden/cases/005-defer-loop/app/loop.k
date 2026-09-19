module app.loop;

priv int live  = 0;
priv int released = 0;

priv int  acquire(int i) { live++; return i == 3 ? 0 : (i == 7 ? 95 : i + 1); }
priv void release(int h)   { (void)h; live--; released++; }

pub int  live_now(void)  { return live; }
pub int  released_now(void) { return released; }
pub void reset(void)         { live = 0; released = 0; }

/* Loop body with mixed exits: `continue`, `break` and `return` go to
   different places, so the cleanup lands INLINE at each exit instead of as a
   ladder (backend §5.5.2). The `[now]` copies at registration. */
pub int run(int n) {
    int total = 0;
    for (int i = 0; i < n; i++) {
        int h = acquire(i);
        defer [now int h] release(h);
        if (h == 0) continue;
        if (h < 0)  break;
        if (h > 90) return -1;
        total += h;
    }
    return total;
}
