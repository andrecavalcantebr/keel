module app.dl;

priv int seen = 0;
priv void note(int v) { seen = v; }
pub  int  last(void) { return seen; }

/* With no list, the body references the variable directly and reads the value
   AT THE EXIT. With `[now]`, the copy is made at registration. The difference
   is semantic. */
pub void late(void) {
    int x = 1;
    defer note(x);               /* reads x at the exit → 3 */
    x = 2;
    x = 3;
}

pub void early(void) {
    int x = 1;
    defer [now int x] note(x);   /* copies at registration → 1 */
    x = 2;
    x = 3;
}

/* `later` is the explicit spelling of the default, and changes nothing. */
pub void late_written(void) {
    int x = 1;
    defer [later] note(x);
    x = 7;
}
