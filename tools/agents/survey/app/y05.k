module app.y05;
import app.lib.other as other types;
pub typedef struct { Item it; } Wrap;
pub int f(Wrap *w) { return w->it.v; }
