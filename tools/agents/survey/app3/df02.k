module app3.df02;
priv int seen = 0;
priv void note(int v) { seen = v; }
pub int f(int n) {
    for (int i = 0; i < n; i++) {
        int x = i;
        if (x == 2) {
            defer [now int x] note(x);
            return x;
        }
    }
    return seen;
}
