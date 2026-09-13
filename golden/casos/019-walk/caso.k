module lst;
import keel.buffer as buffer types;
import keel.slice  as slice  types;

/* Uma lista encadeada não declara `length` nem `ptr`, e ainda assim é
   percorrível: `walk` pede `begin`, `has_next` e `next`, e nada mais.
   Nenhum desses verbos é da base — o protocolo é resolvido pela §4.4. */
pub typedef struct No { i32 v; struct No *prox; } No;
pub typedef struct { No *cabeca; } Lista;
pub typedef struct { No *atual; } cursor;

pub inline cursor begin(Lista *l)                { cursor c = { l->cabeca }; return c; }
pub inline bool   has_next(Lista *l, cursor *c)  { return c->atual != NULL; }
pub inline i32   *next(Lista *l, cursor *c)      { i32 *p = &c->atual->v; c->atual = c->atual->prox; return p; }

pub i32 soma(Lista *l) {
    i32 t = 0;
    walk (i32 *p, lst.cursor c : l) {
        t += *p;
    }
    return t;
}

/* O mesmo statement sobre um contêiner da base: o cursor é `buffer.cursor`,
   declarado uma vez no módulo e escrito sem argumento. */
pub i32 soma_buffer(buffer i32 *xs) {
    i32 t = 0;
    walk (i32 *p, buffer.cursor c : xs) {
        t += *p;
    }
    return t;
}

/* O cursor é objeto do programa: está no escopo do corpo e pode ser lido. */
pub i32 primeiro_par(Lista *l) {
    walk (i32 *p, lst.cursor c : l) {
        if (*p % 2 == 0) return *p;
    }
    return -1;
}
