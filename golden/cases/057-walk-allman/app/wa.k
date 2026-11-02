module app.wa;

/* a list, traversed by walk; the body braces are on their own lines (Allman) */
pub typedef struct Node { i32 v; struct Node *link; } Node;
pub typedef struct { Node *head; } List;
pub typedef struct { Node *current; } cursor;

pub inline cursor begin(List *l) { cursor cur = { l->head }; return cur; }
pub inline bool has_next(List *l, cursor *cur) { return cur->current != NULL; }
pub inline i32 *next(List *l, cursor *cur) { i32 *p = &cur->current->v; cur->current = cur->current->link; return p; }

pub i32 sum(List *l)
{
    i32 t = 0;
    walk (i32 *p, app.wa.cursor c : l)
    {
        t += *p;
    }
    return t;
}
