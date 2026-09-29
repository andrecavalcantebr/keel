module wq;
import keel.buffer as buffer types;
import keel.slice as slice types;

pub size_t f(slice i32 s) { return buffer.length(s); }   /* DIAG: wrong-qualifier */
