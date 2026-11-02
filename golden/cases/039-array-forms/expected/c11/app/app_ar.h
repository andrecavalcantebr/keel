/* app/app_ar.h — generated from app/ar.k by cgen, C11 profile. */
#ifndef APP_APP_AR_H
#define APP_APP_AR_H

#include "app/app_ar.type.h"

#line 7 "app/ar.k"
int app_ar_at_lit(app_ar_Box *b);
int app_ar_at_val(app_ar_Box b);
void app_ar_put(app_ar_Box *b, int i);
int app_ar_cell(app_ar_Grid *gr, int i, int j);
int app_ar_cell_lit(app_ar_Grid *gr);
#line 13 "app/ar.k"
size_t app_ar_len_local(void);
#line 17 "app/ar.k"
size_t app_ar_dim_mod(void);
size_t app_ar_len_param(i32 v[], size_t keel__n);
#line 20 "app/ar.k"
int app_ar_pair(void);

#endif /* APP_APP_AR_H */
