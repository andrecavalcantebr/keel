module p;
import keel.buffer as buffer types;
import keel.slice as slice;
// the protocols come with the prelude (spec §5.1)

pub protocol Seq [IndexPtr, Sliceable];

pub typedef struct Node { i32 v; struct Node *link; } Node;
pub typedef struct { Node *head; } List;
pub typedef struct { Node *current; } cursor;            /* DIAG: shadowed-injected-name */
pub typedef struct { i32 n; } Bare;
pub typedef struct { i32 n; } Shared byref;

pub inline cursor begin(List *l)               { cursor c = { l->head }; return c; }
pub inline bool   has_next(List *l, cursor *c) { return c->current != NULL; }
pub inline i32   *next(List *l, cursor *c)     { i32 *p = &c->current->v; c->current = c->current->link; return p; }

pub size_t count(Traversable C items) { return 0; }

pub size_t use(buffer i32 *xs, List *l, Bare *b) {
    size_t n = count(l);
    slice.slice i32 s = slice.of(xs);
    return n + count(b);                         /* DIAG: protocol-not-satisfied */
}
