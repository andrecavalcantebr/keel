/* app/app_ap.h — gerado de app/ap.k pelo cgen, perfil C11. */
#ifndef APP_APP_AP_H
#define APP_APP_AP_H
#include "app/app_ap.type.h"
#include "keel/keel_range.type.h"

typedef struct keel_buffer_i32 keel_buffer_i32;
#line 8 "app/ap.k"
i32    app_ap_soma_acc(void);
#line 12 "app/ap.k"
void   app_ap_percorre(keel_buffer_i32 *xs);
#line 19 "app/ap.k"
size_t app_ap_soma_range(keel_range r);
#endif
