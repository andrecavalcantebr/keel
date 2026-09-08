/* gen/app/ap.h — gerado de app/ap.k, perfil C11. */
#ifndef APP_AP_H
#define APP_AP_H
#include "keel/prelude.h"
#include "keel/keel_buffer_i32.h"
#include "keel/keel_range.h"

i32    app_ap_soma_acc(void);
void   app_ap_percorre(keel_buffer_i32 *xs);
size_t app_ap_soma_range(keel_range r);
#endif
