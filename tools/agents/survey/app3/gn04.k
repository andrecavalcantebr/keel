module app3.gn04;
import app3.lib.box as box types;
import_c <stdio.h>;
instance box.stack i32;
instance box.stack u8;
pub int f(void) {
    i32 buf[4];
    box.stack i32 s;
    s.cap = 4; s.len = 0; s.ptr = buf;
    return box.push(s, 3) ? (int)box.length(s) : -1;
}
