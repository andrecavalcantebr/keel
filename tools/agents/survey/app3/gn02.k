module app3.gn02;
import app3.lib.box as box types;
instance box.stack i32;
pub size_t count(box.stack i32 *s) { return box.length(s); }
