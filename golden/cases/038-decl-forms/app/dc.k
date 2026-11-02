module app.dc;
import_c <string.h>;

pub struct Box { int a; };
pub union Num { int i; float f; };
pub typedef struct Node Node;
pub struct Node { Node *next; int v; };
priv struct Local { int a; };
pub typedef struct { int a; } P, *PP;

pub int x = 1, y = 2;

pub int total(int n) {
    struct Box b = { n };
    union Num u;
    u.i = b.a;
    struct Local l = { 5 };
    Node nd = { NULL, 3 };
    P p = { 4 };
    return u.i + l.a + nd.v + p.a + x + y + (int)strlen("ab");
}
