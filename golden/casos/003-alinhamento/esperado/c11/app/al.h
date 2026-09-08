/* gen/app/al.h — gerado de app/al.k, perfil C11. */
#ifndef APP_AL_H
#define APP_AL_H
#include "keel/prelude.h"
#include "keel/arena.h"

typedef struct { _Alignas(64) f64 x[8]; } app_al_Vec8;

int app_al_main(int argc, char **argv);
#endif
