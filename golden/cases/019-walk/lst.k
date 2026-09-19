module lst;
import keel.buffer as buffer types;
import keel.slice  as slice  types;

/* A linked list declares neither `length` nor `ptr`, and is traversable all
   the same: `walk` asks for `begin`, `has_next` and `next`, and nothing else.
   None of these verbs is from the base — the protocol is resolved by §4.4. */
pub typedef struct Node { i32 v; struct Node *link; } Node;
pub typedef struct { Node *head; } List;
pub typedef struct { Node *current; } cursor;

pub inline cursor begin(List *l)                { cursor c = { l->head }; return c; }
pub inline bool   has_next(List *l, cursor *c)  { return c->current != NULL; }
pub inline i32   *next(List *l, cursor *c)      { i32 *p = &c->current->v; c->current = c->current->link; return p; }

pub i32 sum(List *l) {
    i32 t = 0;
    walk (i32 *p, lst.cursor c : l) {
        t += *p;
    }
    return t;
}

/* The same statement over a base container: the cursor is `buffer.cursor`,
   declared once for the module and written without an argument. */
pub i32 sum_buffer(buffer i32 *xs) {
    i32 t = 0;
    walk (i32 *p, buffer.cursor c : xs) {
        t += *p;
    }
    return t;
}

/* The cursor is an object of the program: it is in the body's scope and can be read. */
pub i32 first_even(List *l) {
    walk (i32 *p, lst.cursor c : l) {
        if (*p % 2 == 0) return *p;
    }
    return -1;
}
