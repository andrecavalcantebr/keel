/* app/app_df.c — generated from app/df.k by cgen, C11 profile. */
#include "app/app_df.h"
#line 1 "app/df.k"


static int app_df_total = 0;
static int app_df_slots[4] = {0};
static void app_df_add(int d) { app_df_total = app_df_total * 10 + d; }

void app_df_reset(void) { app_df_total = 0; }
int app_df_result(void) { return app_df_total; }
int app_df_slot(int i) { return app_df_slots[i]; }



void app_df_braced(void) {




    app_df_add(3);
#line 14 "app/df.k"
    {
        app_df_add(1);
        app_df_add(2);
    }
#line 19 "app/df.k"
}

void app_df_spread(void) {


    app_df_add(5);
#line 22 "app/df.k"
    app_df_add(
        4);
#line 25 "app/df.k"
}

void app_df_indexed(void) {

    app_df_slots[1] = 2;
#line 28 "app/df.k"
    app_df_slots[1] = 9;
#line 30 "app/df.k"
}

int app_df_returned(void) {



    { int keel__rv0 = app_df_total;
#line 33 "app/df.k"
    {
        app_df_add(6);
    }
    return keel__rv0; }
}
