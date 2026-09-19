/* proof.c — harness. Traversing does not require indexing: the list declares
   neither `length` nor `ptr`, and `walk` still walks it. */

#include "keel.type.h"
#include "keel/keel_buffer_i32.h"
#include "lst.h"
#include <assert.h>
int main(void) {
    lst_Node c = { 3, NULL }, b = { 4, &c }, a = { 1, &b };
    lst_List l = { &a };
    assert(lst_sum(&l) == 8);
    assert(lst_first_even(&l) == 4);

    lst_List empty = { NULL };
    assert(lst_sum(&empty) == 0);
    assert(lst_first_even(&empty) == -1);

    i32 v[4] = { 10, 20, 30, 40 };
    keel_buffer_i32 xs = keel_buffer_i32_of(v, 4);
    assert(lst_sum_buffer(&xs) == 100);

    keel_buffer_i32 empty_buf = keel_buffer_i32_of(v, 0);
    assert(lst_sum_buffer(&empty_buf) == 0);
    return 0;
}
