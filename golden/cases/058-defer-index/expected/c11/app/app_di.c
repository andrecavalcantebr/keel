/* app/app_di.c — generated from app/di.k by cgen, C11 profile. */
#include "app/app_di.h"
#line 1 "app/di.k"


static int app_di_seen = -1;
int app_di_last(void) { return app_di_seen; }



void app_di_clear_var(size_t i) {
    i32 v[3] = {7, 8, 9};


#line 11 "app/di.k"
    v[keel_index(i, 3)] = 0;
#line 10 "app/di.k"
    app_di_seen = v[0];
#line 12 "app/di.k"
}



int app_di_take_var(size_t i) {
    i32 v[3] = {7, 8, 9};


    { int keel__rv0 = v[0];
#line 19 "app/di.k"
    v[keel_index(i, 3)] = 0;
#line 18 "app/di.k"
    app_di_seen = v[0];
#line 20 "app/di.k"
    return keel__rv0; }
}



void app_di_clear_now(void) {
    i32 v[3] = {7, 8, 9};
    size_t k = 1;

    struct { size_t k; } keel__c0 = { k };
    k = 2;
#line 29 "app/di.k"
    v[keel_index(keel__c0.k, 3)] = 0;
#line 28 "app/di.k"
    app_di_seen = v[1];
#line 31 "app/di.k"
}


void app_di_clear_lit(void) {
    i32 v[3] = {7, 8, 9};


#line 37 "app/di.k"
    v[1] = 0;
#line 36 "app/di.k"
    app_di_seen = v[0] * 10 + v[1];
#line 38 "app/di.k"
}


void app_di_two_vars(size_t i, size_t j) {
    i32 v[3] = {7, 8, 9};





#line 44 "app/di.k"
    {
        v[keel_index(i, 3)] = 1;
        v[keel_index(j, 3)] = 2;
    }
#line 43 "app/di.k"
    app_di_seen = v[0] * 100 + v[1] * 10 + v[2];
#line 48 "app/di.k"
}
