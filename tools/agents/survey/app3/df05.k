module app3.df05;
priv int total = 0;
priv void add(int d) { total += d; }
pub int f(void) {
    defer add(
        4);  /* spread call, comment kept */
    add(5);
    return total;
}
