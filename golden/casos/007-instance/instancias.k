module instancias;
import pilha;

/* Coloca, não gera: a geração acontece pelo uso; `instance` decide onde moram
   os corpos das funções `pub` não-inline (§4.9). */
instance pilha.stack i32;
