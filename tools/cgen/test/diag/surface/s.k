module s;
import keel.slice as slice types;
import keel.outcome as outcome types;

pub void use(slice const char t, outcome void v, slice i32 u) {
    char c = slice.get(t, 0);
    slice.set(t, 0, c);                     /* DIAG: verb-not-in-instance */
    outcome const char o = slice.at(t, 1);  /* DIAG: verb-not-in-instance */
    outcome.win(v);
    outcome.value(v);                       /* DIAG: verb-not-in-instance */
    slice.set(u, 0, 1);
}
