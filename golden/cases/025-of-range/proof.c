/* proof.c — the case's harness. NOT transpiler output: it is the program
   that exercises the generated public interface and asserts the behaviour.
   Compiled together with expected/<perfil>/app/ofr.c. */

#include "keel.type.h"
#include "app/app_ofr.h"
#include "keel/keel_range.h"
#include <stdio.h>

int main(void) {
    keel_range r = keel_range_of(1, 4);

    keel_slice_i32 w = app_ofr_whole();                 /* of(v): the whole array */
    if (w.len != 6 || w.ptr[0] != 10) return 1;
    keel_slice_i32 m = app_ofr_middle(2, 5);            /* of(v, a, b) */
    if (m.len != 3 || m.ptr[0] != 12) return 2;
    keel_slice_i32 br = app_ofr_by_range(r);            /* of(v, r) */
    if (br.len != 3 || br.ptr[0] != 11) return 3;
    if (app_ofr_same(w).len != 6) return 4;             /* of(s) */
    keel_slice_i32 s = app_ofr_sub(w, r);               /* of(s, r) */
    if (s.len != 3 || s.ptr[2] != 13) return 5;
    keel_slice_i32 fb = app_ofr_from_buffer(r);         /* of(b, r): as_slice1 */
    if (fb.len != 3 || fb.ptr[0] != 11) return 6;

    if (app_ofr_count() != 6) return 7;                 /* keel.length */
    if (app_ofr_second() != 11) return 8;               /* array.get */
    app_ofr_put(5, 50);                                 /* array.set */
    if (*app_ofr_at_ptr(5) != 50) return 9;             /* array.ptr */
    if (app_ofr_checked(5) != 50) return 10;            /* array.at, inside */
    if (app_ofr_checked(6) != -1) return 11;            /* array.at, outside: every build */

    if (app_ofr_inverted() != 0) return 12;             /* range.of(5, 2) is empty */
    if (app_ofr_literal() != 3) return 13;              /* 2..5 */

    puts("ok");
    return 0;
}
