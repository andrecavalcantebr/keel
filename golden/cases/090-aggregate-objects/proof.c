#include <assert.h>
#include "aggregate.h"

int main(void) {
    assert(aggregate_s0.x == 0);
    assert(aggregate_u1 == 0);
    aggregate_s0.x = 7;
    aggregate_u1 = &aggregate_us[1];
    aggregate_u1->y = 9;
    assert(aggregate_s0.x == 7);
    assert(aggregate_u0.x == 0);
    assert(aggregate_us[1].y == 9);
    return 0;
}
