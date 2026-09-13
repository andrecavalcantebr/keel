/* prova.c — arnês. Percorrer não exige indexar: a lista não declara `length`
   nem `ptr`, e mesmo assim `walk` a percorre. */
#include "lst/lst.h"
#include <assert.h>

int main(void) {
    lst_No c = { 3, NULL }, b = { 4, &c }, a = { 1, &b };
    lst_Lista l = { &a };
    assert(lst_soma(&l) == 8);
    assert(lst_primeiro_par(&l) == 4);

    lst_Lista vazia = { NULL };
    assert(lst_soma(&vazia) == 0);
    assert(lst_primeiro_par(&vazia) == -1);

    i32 v[4] = { 10, 20, 30, 40 };
    keel_buffer_i32 xs = keel_buffer_i32_of(v, 4);
    assert(lst_soma_buffer(&xs) == 100);

    keel_buffer_i32 vazio = keel_buffer_i32_of(v, 0);
    assert(lst_soma_buffer(&vazio) == 0);
    return 0;
}
