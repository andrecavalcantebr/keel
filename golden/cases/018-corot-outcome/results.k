module results;
import keel.corot as corot types;
import keel.outcome as outcome types;

pub corot state(i32 code) {
    corot r = {0};
    if (code > 0) return corot.fault(r, code);
    if (code == 0) return corot.again(r);
    return corot.win(r);
}

pub bool is_fault(i32 code) {
    corot r = state(code);
    return corot.faulted(r);
}

pub outcome void final(bool failure, i32 code) {
    outcome void r = {0};
    if (failure) return outcome.fail(r, code);
    return outcome.win(r);
}

pub i32 repair(i32 code) {
    outcome i32 origin = {0};
    outcome.fail(origin, code);
    outcome i32 r = origin else 5;
    return outcome.value(r);
}

/* The verb's argument is evaluated once, and before the cleanup: the returned
   code is one and the outer counter ends at two. */
pub corot evaluate_cleanup(i32 *counter) {
    corot r = {0};
    defer ++*counter;
    return corot.fault(r, ++*counter);
}
