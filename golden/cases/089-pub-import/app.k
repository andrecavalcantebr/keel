module app;
import facade as f;

pub i32 run(void) {
    f.mid.leaf.Count n = f.mid.leaf.value();
    return n + f.mid.leaf.BIAS;
}
