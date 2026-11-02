module app3.df01;
pub int f(int n) {
    int t = 0;
    int i = 0;
    while (i < n) {
        defer t++;
        i++;
        if (i == 2) break;
    }
    return t;
}
