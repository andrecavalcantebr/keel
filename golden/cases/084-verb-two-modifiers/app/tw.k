module app.tw;
import twos types;
import_c <stdio.h>;

pub int main(int argc, char **argv) {
    i32 v[4] = {0};
    out i32 o = {4, v};
    twos.fill(o, 7);
    tgt i32 d = {v};
    if (twos.run(d, 3) != 7) return 1;
    puts("ok");
    return 0;
}
