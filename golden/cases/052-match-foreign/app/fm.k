module app.fm;
import app.lib.other as other types;

pub int foreign(Gate *g) {
    int r = 0;
    match (g->ph) {
        ON:
            r = 1;
        OFF:
            r = 2;
    }
    return r;
}
