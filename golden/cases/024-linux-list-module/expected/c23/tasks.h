/* tasks.h — generated from tasks.k by cgen, C23 profile. */
#ifndef TASKS_H
#define TASKS_H

#include "tasks.type.h"

#line 19 "tasks.k"
static inline tasks_cursor tasks_begin(tasks_queue *q);
static inline bool   tasks_has_next(tasks_queue *q, tasks_cursor *c);
static inline tasks_task  *tasks_next(tasks_queue *q, tasks_cursor *c);
#line 23 "tasks.k"
int tasks_main(void);

#line 19 "tasks.k"
static inline tasks_cursor tasks_begin(tasks_queue *q)               { tasks_cursor c = { q->head.next }; return c; }
static inline bool   tasks_has_next(tasks_queue *q, tasks_cursor *c) { return c->at != &q->head; }
static inline tasks_task  *tasks_next(tasks_queue *q, tasks_cursor *c)     { tasks_task *t = list_entry(c->at, tasks_task, link); c->at = c->at->next; return t; }

#endif /* TASKS_H */
