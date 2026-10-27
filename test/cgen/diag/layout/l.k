module l;
import keel.outcome as outcome types;
import keel.buffer as buffer types;
pub struct No { i32 v; outcome No next; };              /* DIAG: layout-cycle */
pub struct Ok { i32 v; outcome buffer Ok kids; };
