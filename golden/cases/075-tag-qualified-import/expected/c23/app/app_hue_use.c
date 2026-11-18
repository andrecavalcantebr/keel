/* app/app_hue_use.c — generated from app/hue_use.k by cgen, C23 profile. */
#include "app/app_hue_use.h"
#include "app/app_hue.h"
#line 1 "app/hue_use.k"



int app_hue_use_green(void) { return app_hue_Hue_GREEN; }

app_hue_Hue app_hue_use_next(app_hue_Hue h) {
    if (h == app_hue_Hue_RED) return app_hue_Hue_GREEN;
    return app_hue_Hue_BLUE;
}
