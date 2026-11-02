module app.d01;
pub struct Box { int a; };
pub int f(struct Box *b) { return b->a; }
