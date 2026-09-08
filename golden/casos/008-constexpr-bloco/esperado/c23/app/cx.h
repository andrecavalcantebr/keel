/* gen/app/cx.h — gerado de app/cx.k, perfil C23. */
#ifndef APP_CX_H
#define APP_CX_H
#include "keel/prelude.h"

typedef struct { int N; int MAX; } app_cx_Cfg;

constexpr size_t app_cx_MAX = 4;

int app_cx_soma(app_cx_Cfg *c);
int app_cx_outro(void);
int app_cx_main(int argc, char **argv);
#endif
