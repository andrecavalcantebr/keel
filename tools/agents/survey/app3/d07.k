module app3.d07;
pub typedef struct Node Node;
pub struct Node { Node *next, *prev; int v; };
pub int f(Node *n) { return n->next ? n->next->v : n->v; }
