/* app/app_tbl.type.h — generated from app/tbl.k by cgen, C11 profile. */
#ifndef APP_APP_TBL_TYPE_H
#define APP_APP_TBL_TYPE_H

#include "keel.type.h"

#line 3 "app/tbl.k"
typedef enum app_tbl_Op {app_tbl_Op_NOP, app_tbl_Op_ADD, app_tbl_Op_SUB} app_tbl_Op;
#line 5 "app/tbl.k"
typedef struct app_tbl_Row { app_tbl_Op op; i32 arg; } app_tbl_Row;

#endif /* APP_APP_TBL_TYPE_H */
