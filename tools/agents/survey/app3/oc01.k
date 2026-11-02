module app3.oc01;
import keel.corot as corot types;
pub corot step(int *n) {
    corot r = {0};
    if (*n > 0) { (*n)--; return corot.again(r); }
    return corot.win(r);
}
pub int drive(int n) {
    int left = n;
    corot r = {0};
    while (corot.ongoing(r = step(&left))) { }
    return corot.faulted(r);
}
