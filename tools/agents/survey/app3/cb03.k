module app3.cb03;
import keel.tagged as tagged types;
import keel.buffer as buffer;
pub tags Mode [ON, OFF];
pub int f(tagged Mode void m, buffer.buffer i32 *xs) {
    int t = 0;
    match (m) {
        ON:
            foreach (i32 x, size_t i : xs) {
                defer t++;
                if (x == 0) break;
            }
            break;
        OFF:
            t = -1;
    }
    return t;
}
