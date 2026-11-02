module app.sf;
import keel.slice as slice;

/* constexpr that cites constexpr of the module: each INIT names the one before,
   and every use is renamed (backend §9.2) */
pub constexpr size_t A = 2;
pub constexpr size_t B = A * A;
pub constexpr size_t C = B + A;

pub size_t cval(void) { return C; }

/* a list, traversed by walk: begin, has_next and next, and nothing else */
pub typedef struct Node { i32 v; struct Node *link; } Node;
pub typedef struct { Node *head; } List;
pub typedef struct { Node *current; } cursor;

pub inline cursor begin(List *l) { cursor cur = { l->head }; return cur; }
pub inline bool has_next(List *l, cursor *cur) { return cur->current != NULL; }
pub inline i32 *next(List *l, cursor *cur) { i32 *p = &cur->current->v; cur->current = cur->current->link; return p; }

pub i32 sum(List *l) {
    i32 t = 0;
    walk (i32 *p, app.sf.cursor c : l) {
        t += *p;
    }
    return t;
}

pub i32 allman(slice.slice i32 s)
{
    i32 t = 0;
    foreach (i32 x, size_t i : s)
    {
        t += x - (i32)i;
    }
    return t;
}
