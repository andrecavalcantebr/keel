module app.ob;
import boxes types;
import keel.outcome as outcome types;
import_c <stdio.h>;

pub int main(int argc, char **argv) {
    outcome item r = {0};
    item v = {5};
    outcome.win(r, v);
    if (!outcome.ok(r)) return 1;
    if (outcome.value(r).x != 5) return 2;
    puts("ok");
    return 0;
}
