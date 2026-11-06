/* app/app_dag.c — generated from app/dag.k by cgen, C11 profile. */
#include "app/app_dag.h"
#line 1 "app/dag.k"





















int app_dag_crosses(int x) {
    int keel__rv0;
    if (x) {
#ifdef A
#  ifdef B
        if (x) {
#  else
        if (!x) {
#  endif
#else
        if (x > 1) {
#endif
            x++;
        }
        if (x) {
#line 23 "app/dag.k"
    printf("cleanup %d\n", x);
#line 36 "app/dag.k"
        goto out; }
    }
out:
    keel__rv0 = x; goto keel__e0;
#line 23 "app/dag.k"
keel__e0: printf("cleanup %d\n", x);
#line 40 "app/dag.k"
    return keel__rv0; }




int app_dag_h(int a, int b) { return a + b; }
int app_dag_wrapped(int x) {
    return app_dag_h(
#ifdef D
        x, 1
#else
        x, 2
#endif
    );
}
