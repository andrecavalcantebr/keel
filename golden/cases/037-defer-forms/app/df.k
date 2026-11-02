module app.df;

priv int total = 0;
priv int slots[4] = {0};
priv void add(int d) { total = total * 10 + d; }

pub void reset(void) { total = 0; }
pub int result(void) { return total; }
pub int slot(int i) { return slots[i]; }

/* `defer` keeps the author's writing (backend §5.5, rule 4): braces, line breaks and
   indices stay as written, and each line of the body maps to its own .k line. */
pub void braced(void) {
    defer {
        add(1);
        add(2);
    }
    add(3);
}

pub void spread(void) {
    defer add(
        4);
    add(5);
}

pub void indexed(void) {
    defer slots[1] = 9;
    slots[1] = 2;
}

pub int returned(void) {
    defer {
        add(6);
    }
    return total;
}
