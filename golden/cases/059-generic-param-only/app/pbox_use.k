module app.pbox_use;
import app.pbox types;

pub size_t probe_cap(void) { box(3) i32 b; return cap(b); }
pub i32 probe_first(void) { box(3) i32 b; b.v[0] = 4; b.v[1] = 5; b.v[2] = 6; return ends(b).first; }
pub i32 probe_last(void) { box(3) i32 b; b.v[0] = 4; b.v[1] = 5; b.v[2] = 6; return ends(b).last; }
