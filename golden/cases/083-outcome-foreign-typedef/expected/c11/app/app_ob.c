/* app/app_ob.c — generated from app/ob.k by cgen, C11 profile. */
#include "app/app_ob.h"
#include "boxes.h"
#include "keel/keel_outcome_boxes_item.h"
#line 1 "app/ob.k"





int app_ob_main(int argc, char **argv) {
    keel_outcome_boxes_item r = {0};
    boxes_item v = {5};
    keel_outcome_boxes_item_win1(&r, v);
    if (!keel_outcome_boxes_item_ok(r)) return 1;
    if (keel_outcome_boxes_item_value(r).x != 5) return 2;
    puts("ok");
    return 0;
}
