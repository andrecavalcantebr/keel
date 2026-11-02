module app3.mt04;
import app3.lib.other as other types;
pub int f(other.Phase p) {
    int r = 0;
    match (p) {
        ON: r = 1;
        OFF: r = 0;
    }
    return r;
}
