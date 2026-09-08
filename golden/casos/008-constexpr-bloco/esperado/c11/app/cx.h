/* gen/app/cx.h — gerado de app/cx.k, perfil C11. */
#ifndef APP_CX_H
#define APP_CX_H
#include "keel/prelude.h"

typedef struct { int N; int MAX; } app_cx_Cfg;

#define app_cx_MAX ((size_t)4)
static const size_t app_cx_MAX__chk = 4;

int app_cx_soma(app_cx_Cfg *c);
int app_cx_outro(void);
int app_cx_main(int argc, char **argv);
#endif
