module app3.cb04;
import keel.buffer as buffer types;
import app3.lib.box as box types;
instance box.stack i32;
pub bool fill(box.stack i32 *s, i32 n) {
    for (i32 i = 0; i < n; i++)
        if (!box.push(s, i)) return false;
    return box.length(s) == (size_t)n;
}
