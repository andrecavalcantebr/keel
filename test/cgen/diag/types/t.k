module t;
import keel.buffer as buffer types;

pub i32 f(void) {
    buffer int x;                                /* DIAG: c-type-as-argument */
    buffer unsigned long y;                      /* DIAG: c-type-as-argument */
    restrict buffer i32 *p;                      /* DIAG: restrict-on-container */
    buffer buffer i32 grid;
    buffer i32 e = buffer.get(grid, 0);          /* DIAG: element-copy-in-get-set */
    return 0;
}
