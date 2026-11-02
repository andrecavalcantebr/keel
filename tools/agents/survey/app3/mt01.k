module app3.mt01;
import keel.tagged as tagged types;
pub tags Cycle [A, B, C];
pub typedef struct { tagged Cycle void state; int n; } Machine;
pub int run(Machine *m, int steps) {
    int r = 0;
    for (int i = 0; i < steps; i++) {
        match (m->state) {
            A: r += 1; break;
            B: r += 2; break;
            C: r += 3;
        }
    }
    return r;
}
