module app.y01;
import app.lib.other as other types;
pub int f(Gate *g) {
    int r = 0;
    match (g->ph) {
        ON:
            r = 1;
        OFF:
            r = 2;
    }
    return r;
}
