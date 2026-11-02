module app3.df03;
priv int trail = 0;
priv void mark(int d) { trail = trail * 10 + d; }
pub int f(int k) {
    {
        defer {
            mark(1);
        }
        if (k == 0) goto out;
        mark(2);
    }
    return 0;
out:
    return trail;
}
