module app.mx_use;
import mx as mx types;

pub int main(void) {
    mx.box(2, 3) v = mx.zeros();
    mx.set(v, 1, 2, 5.0);
    if (mx.get(v, 0, 0) != 0.0) return 1;
    if (mx.get(v, 1, 2) != 5.0) return 1;
    return 0;
}
