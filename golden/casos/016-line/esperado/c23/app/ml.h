/* gen/app/ml.h — gerado de app/ml.k, perfil C23. */
#ifndef APP_ML_H
#define APP_ML_H
#include "keel/prelude.h"
#include "keel/keel_buffer_i32.h"

typedef struct { int v; } app_ml_R;

int app_ml_usa(app_ml_R *r, keel_buffer_i32 *xs);
#endif
