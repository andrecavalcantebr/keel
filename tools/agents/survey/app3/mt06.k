module app3.mt06;
import keel.tagged as tagged types;
pub tags Cycle [A, B];
pub int f(tagged Cycle void s) {
    match (s) {
        A: return 1;
        B: return 2;
    }
    return 0;
}
