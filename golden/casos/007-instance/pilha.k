module pilha type T;

/* Não menciona `T` nem `stack` → emitida UMA VEZ, no módulo, e o nome não leva
   o argumento (§4.9). É o caso de `outcome.OK`. */
pub constexpr i32 VAZIA = -1;

pub modifier stack byref { size_t cap, len; T *ptr; }

/* Menciona `stack`, embora não escreva `T` → pertence à instância. */
pub inline size_t length(stack *s) { return s->len; }

/* `pub` sem `inline` → sai `extern` na instância, e o corpo precisa de dono. */
pub bool empurra(stack *s, T v);
