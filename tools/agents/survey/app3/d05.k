module app3.d05;
pub int len(const char *s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}
