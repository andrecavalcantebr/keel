/* app/app_reg.c — gerado de app/reg.k pelo cgen, perfil C11. */
#include "app/app_reg.h"
#include "keel/keel_slice_const_char.h"
#include "keel/keel_outcome_keel_slice_const_char.h"
#include "keel/keel_outcome_f64.h"
#line 1 "app/reg.k"













app_reg_Person app_reg_people[4];

static const char *app_reg_NAMES[3]   = { "ana", "bo", "cid" };
static size_t      app_reg_LENS[3]    = { 3, 2, 3 };
static const char *app_reg_ADDRS[3]   = { "rd one", "", "rd three" };
static size_t      app_reg_ADDRLENS[3] = { 6, 0, 8 };
static f64         app_reg_BIRTHS[3]  = { 1.5, 2.25, 3.5 };


static keel_outcome_keel_slice_const_char app_reg_read_name(size_t i) {
    keel_outcome_keel_slice_const_char r = {0};
    if (i >= 3) return keel_outcome_keel_slice_const_char_none(&r);
    return keel_outcome_keel_slice_const_char_win1(&r, keel_slice_const_char_from(app_reg_NAMES[i], app_reg_LENS[i]));
}

static keel_outcome_keel_slice_const_char app_reg_read_address(size_t i) {
    keel_outcome_keel_slice_const_char r = {0};
    if (i >= 3 || app_reg_ADDRLENS[i] == 0) return keel_outcome_keel_slice_const_char_none(&r);
    return keel_outcome_keel_slice_const_char_win1(&r, keel_slice_const_char_from(app_reg_ADDRS[i], app_reg_ADDRLENS[i]));
}

static keel_outcome_f64 app_reg_read_birth(size_t i) {
    keel_outcome_f64 r = {0};
    if (i >= 3 || i == 2) return keel_outcome_f64_none(&r);
    return keel_outcome_f64_win1(&r, app_reg_BIRTHS[i]);
}





size_t app_reg_load(size_t n) {
    keel_outcome_keel_slice_const_char name = {0};
    keel_outcome_keel_slice_const_char address = {0};
    keel_outcome_f64 birth = {0};
    size_t k = 0;

    for (size_t i = 0; i < n && i < 4; i++) {
        name = app_reg_read_name(i); if (keel_outcome_keel_slice_const_char_failed(name)) break;
        address = app_reg_read_address(i); if (keel_outcome_keel_slice_const_char_failed(address)) keel_outcome_keel_slice_const_char_win1(&address, keel_slice_const_char_from("", 0));
        birth = app_reg_read_birth(i); if (keel_outcome_f64_failed(birth)) keel_outcome_f64_win1(&birth, 0.0);

        app_reg_people[i].name    = keel_outcome_keel_slice_const_char_value(name);
        app_reg_people[i].address = keel_outcome_keel_slice_const_char_value(address);
        app_reg_people[i].birth   = keel_outcome_f64_value(birth);
        k++;
    }
    return k;
}
