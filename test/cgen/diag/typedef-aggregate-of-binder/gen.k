module gen dim N type T;

pub typedef struct { T first; T last; } Pair;      /* DIAG: typedef-aggregate-of-binder */
pub typedef union { T v; u8 raw; } Cell;           /* DIAG: typedef-aggregate-of-binder */
pub typedef struct { size_t i; } cursor;
pub typedef T Elem;
pub typedef T *Ref;
pub modifier box byref { T v[N]; }
