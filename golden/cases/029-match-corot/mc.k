module mc;
import keel.corot as corot types;

/* The exhaustive set of a `corot` is what its `tag` returns, `Status` (spec
   §4.9): the code is read through the verb, and two failure codes are the
   same arm. */
pub i32 classify(corot r) {
    i32 n = 0;
    match (r) {
        SUCCESS:
            n = 1;
            break;
        ONGOING:
            n = 2;
            break;
        FAILED:
            n = corot.code(r);
    }
    return n;
}

/* An operand whose declared type is a set is read as it stands. */
pub i32 rank(corot.Status s) {
    i32 n = 0;
    match (s) {
        SUCCESS:
            n = -1;
            break;
        ONGOING:
            n = 0;
            break;
        FAILED:
            n = 1;
    }
    return n;
}
