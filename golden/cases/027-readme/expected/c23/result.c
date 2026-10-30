/* result.c — generated from result.k by cgen, C23 profile. */
#include "result.h"
#include "keel/keel_outcome_i16.h"
#line 1 "result.k"



keel_outcome_i16 result_accept(i16 entry) {
    keel_outcome_i16 r = {0};
    if (entry < 0) return keel_outcome_i16_fail(&r, entry);
    return keel_outcome_i16_win1(&r, entry);
}

i16 result_get_value(i16 entry) {
    keel_outcome_i16 res = result_accept(entry); if (keel_outcome_i16_failed(res)) keel_outcome_i16_win1(&res, 0);
    return keel_outcome_i16_value(res);
}
