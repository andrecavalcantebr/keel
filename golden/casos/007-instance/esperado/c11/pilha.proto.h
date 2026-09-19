/* gen/pilha.h — o `.h` do módulo genérico, uma vez. A constante não menciona
   parâmetro nem modificador, então não pertence a instância nenhuma e o nome
   não leva o argumento (backend §4.4.1). */
#ifndef PILHA_H
#define PILHA_H
#include "pilha.type.h"

#define pilha_VAZIA ((i32)-1)
static const i32 pilha_VAZIA__chk = -1;
#endif /* PILHA_H */
