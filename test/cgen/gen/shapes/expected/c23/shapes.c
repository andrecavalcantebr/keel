/* shapes.c — generated from shapes.k by cgen, C23 profile. */
#include "shapes.h"
#line 1 "shapes.k"










int shapes_counter = 0;
static int shapes_scale = 3;


static int shapes_weight(shapes_Point *p) {
    return p->x * shapes_scale;
}

int shapes_sum(shapes_Point *p) {
    return shapes_weight(p) + p->y + shapes_LIMIT;
}
