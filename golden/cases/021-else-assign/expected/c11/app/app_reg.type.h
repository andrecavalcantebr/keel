/* app/app_reg.type.h — gerado de app/reg.k pelo cgen, perfil C11. */
#ifndef APP_APP_REG_TYPE_H
#define APP_APP_REG_TYPE_H
#include "keel.type.h"
#include "keel/keel_slice_const_char.type.h"

#line 8 "app/reg.k"
typedef struct app_reg_Person {
    keel_slice_const_char name;
    keel_slice_const_char address;
    f64 birth;
} app_reg_Person;
#endif /* APP_APP_REG_TYPE_H */
