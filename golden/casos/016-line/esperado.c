/* gen/app/ln.c — perfil C23.
   O gerador mantém dois contadores — a linha do .k e a do arquivo de saída — e
   emite `#line` sempre que divergem (backend §6). Aqui o cabeçalho gerado tem
   quatro linhas a mais que o fonte, então um `#line` ressincroniza antes do
   corpo. NÃO COMPILA de propósito: ver VERIFICA. */
#include "keel/prelude.h"
#include "keel/keel_buffer_i32.h"

#line 7 "app/ln.k"
void app_ln_erra(keel_buffer_i32 *b) {
    f32 *p = keel_buffer_i32_ptr(b);
    (void)p;
}
