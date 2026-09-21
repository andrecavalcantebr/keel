/* SPDX-License-Identifier: GPL-2.0-only */
module tasks;
import_c <assert.h>;
import_c <stdio.h>;
import linux.list as list;

/* list.h is intrusive: the node carries the link, and the list knows nothing
   of the type around it. `task` is a keel name — mangled — with a C field. */
pub typedef struct task { i32 id; struct list_head link; } task;

/* The keel side of the same list: `walk` asks for `begin`, `has_next` and
   `next`, declared here by the program, as in case 019. `next` is where the
   two worlds meet: `list_entry` is a list.h macro, and `task`, written inside
   its arguments, is a keel name. */
pub typedef struct { struct list_head head; } queue;
pub typedef struct { struct list_head *at; } cursor;

pub inline cursor begin(queue *q)               { cursor c = { q->head.next }; return c; }
pub inline bool   has_next(queue *q, cursor *c) { return c->at != &q->head; }
pub inline task  *next(queue *q, cursor *c)     { task *t = list_entry(c->at, task, link); c->at = c->at->next; return t; }

int main(void) {
    queue q;
    list.INIT_LIST_HEAD(&q.head);

    /* insertion: the nodes live in the caller's storage */
    task pool[5];
    for (i32 i = 0; i < 5; i++) {
        pool[i].id = i + 1;
        list.list_add_tail(&pool[i].link, &q.head);
    }

    /* removal while traversing: list.h's own safe iterator */
    task *t, *n;
    list_for_each_entry_safe(t, n, &q.head, link) {
        if (t->id % 2 == 0) list.list_del(&t->link);
    }

    /* listing, by keel's walk */
    i32 want[] = { 1, 3, 5 };
    size_t k = 0;
    walk (task *p, tasks.cursor c : q) {
        printf("%d\n", (int)p->id);
        assert(p->id == want[k++]);
    }
    assert(k == 3);

    /* and by list.h's macro: both traversals must agree */
    k = 0;
    list_for_each_entry(t, &q.head, link) {
        assert(t->id == want[k++]);
    }
    assert(k == 3);
    return 0;
}
