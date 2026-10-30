module result;
import keel.outcome as outcome types;

pub outcome i16 accept(i16 entry) {
    outcome i16 r = {0};
    if (entry < 0) return outcome.fail(r, entry);
    return outcome.win(r, entry);
}

pub i16 get_value(i16 entry) {
    outcome i16 res = accept(entry) else 0;
    return outcome.value(res);
}
