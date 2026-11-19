/* app/app_hue2_use.c — generated from app/hue2_use.k by cgen, C23 profile. */
#include "app/app_hue2_use.h"
#include "app/app_hue2.h"
#line 1 "app/hue2_use.k"



int app_hue2_use_level(void) { return app_hue2_Mode_LOW; }

app_hue2_Mode app_hue2_use_step(app_hue2_Mode m) {
    if (m == app_hue2_Mode_OFF) return app_hue2_Mode_LOW;
    return app_hue2_Mode_HIGH;
}
