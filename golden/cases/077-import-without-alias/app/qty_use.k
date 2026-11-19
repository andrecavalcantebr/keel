module app.qty_use;
import app.qty;

/* no alias: the qualifier is the module's own dotted name, in full (spec
   §4.1 regra 5) */
pub int total(void) { return app.qty.SEVEN + app.qty.triple(2); }
