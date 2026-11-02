module app3.df07;
priv int total = 0;
priv void add(int d) { total += d; }
pub int f(int n) {
    for (int i = 0; i < n; i++) {
        defer add(i);
        if (i % 2) continue;
        total += 100;
    }
    return total;
}
