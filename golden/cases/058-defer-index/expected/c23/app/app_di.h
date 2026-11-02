/* app/app_di.h — generated from app/di.k by cgen, C23 profile. */
#ifndef APP_APP_DI_H
#define APP_APP_DI_H

#include "app/app_di.type.h"

#line 4 "app/di.k"
int app_di_last(void);
#line 8 "app/di.k"
void app_di_clear_var(size_t i);
#line 16 "app/di.k"
int app_di_take_var(size_t i);
#line 25 "app/di.k"
void app_di_clear_now(void);
#line 34 "app/di.k"
void app_di_clear_lit(void);
#line 41 "app/di.k"
void app_di_two_vars(size_t i, size_t j);

#endif /* APP_APP_DI_H */
