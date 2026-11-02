module app3.wk02;
import keel.buffer as buffer;
pub i32 total(buffer.buffer i32 *xs) {
    i32 t = 0;
    walk (i32 *p, buffer.cursor c : xs) {
        t += *p;
    }
    return t;
}
