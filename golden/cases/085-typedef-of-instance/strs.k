module strs;
import keel.buffer as buffer types;
import keel.slice as slice;

pub typedef buffer u8 text;
pub typedef slice.slice const u8 view;

pub inline size_t size(text *t) { return buffer.length(*t); }
