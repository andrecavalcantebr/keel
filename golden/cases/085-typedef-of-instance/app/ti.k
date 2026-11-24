module app.ti;
import strs types;
import keel.buffer as buffer;
import keel.slice as slice;
import_c <stdio.h>;

pub int main(int argc, char **argv) {
    text t = {0};
    view v = {0};
    if (buffer.length(t) != 0) return 1;
    if (slice.length(v) != 0) return 2;
    if (strs.size(&t) != 0) return 3;
    puts("ok");
    return 0;
}
