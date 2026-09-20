/* app/app_cfg.h — generated from app/cfg.k by cgen, C11 profile. */
#ifndef APP_APP_CFG_H
#define APP_APP_CFG_H
#include "app/app_cfg.type.h"
#include "keel/keel_outcome_i32.type.h"

typedef struct keel_arena keel_arena;
#line 11 "app/cfg.k"
keel_outcome_i32 app_cfg_sum(keel_arena *a, const char *path);
#line 30 "app/cfg.k"
keel_outcome_i32 app_cfg_sum_scratch(const char *path);
#endif /* APP_APP_CFG_H */
