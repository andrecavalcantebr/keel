module app3.wk01;
import keel.buffer as buffer;
import keel.slice  as slice;

pub typedef struct Node { i32 v; struct Node *link; } Node;
pub typedef struct { Node *head; } List;
pub typedef struct { Node *current; } cursor;

pub inline cursor begin(List *l) { cursor c = { l->head }; return c; }
pub inline bool has_next(List *l, cursor *c) { return c->current != NULL; }
pub inline i32 *next(List *l, cursor *c) {
    i32 *p = &c->current->v;
    c->current = c->current->link;
    return p;
}

pub i32 find(List *l, i32 want) {
    walk (i32 *p, app3.wk01.cursor c : l) {
        if (*p == want)
            return *p;
    }
    return -1;
}
