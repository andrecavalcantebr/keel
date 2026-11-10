module app.mx_use;
import mx as mx types;

pub int main(void) {
    mx.box(2, 3) a = {0};
    mx.set(a, 0, 2, 7.0);
    mx.box(3, 2) t = mx.transpose(a);
    if (mx.get(t, 2, 0) != 7.0) return 1;
    return 0;
}
