module pt;
import keel.tagged as tagged types;

priv tags Mode [OFF = 1, ON = 3, AUTO = 7];

/* The set is private to keel, and the enum is in the .type.h all the same: the
   instance of `tagged` names it (backend §5.6, rule 1; D29). */
pub i32 trail(void) {
    tagged Mode void m = {0};
    tagged.mark(m, OFF);
    i32 n = 0;
    bool done = false;

    while (!done) {
        match (m) {
            OFF:
                n += 1;
                tagged.mark(m, ON);
            ON:
                n += 10;
                tagged.mark(m, AUTO);
            AUTO:
                n += 100;
                done = true;
        }
    }
    return n;
}
