module app.d03;
pub typedef struct Node Node;
pub struct Node { Node *next; int v; };
