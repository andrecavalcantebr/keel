module use;
import lib;

// the importer is analysed all the same: its own error is reported beside the import's
pub void go(invalidates lib.Box *b) {
    i32 *p = lib.raw(b);
    lib.clear(b);
    i32 v = *p;                                   /* DIAG: child-region-after-invalidation */
    (void)v;
}
