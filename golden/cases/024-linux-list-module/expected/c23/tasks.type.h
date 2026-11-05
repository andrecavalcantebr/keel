/* tasks.type.h — generated from tasks.k by cgen, C23 profile. */
#ifndef TASKS_TYPE_H
#define TASKS_TYPE_H

#include "keel.type.h"
#include "linux/linux_list.type.h"

#line 3 "tasks.k"
#include <assert.h>
#include <stdio.h>
#line 9 "tasks.k"
typedef struct tasks_task { i32 id; struct linux_list_list_head link; } tasks_task;
#line 15 "tasks.k"
typedef struct tasks_queue { struct linux_list_list_head head; } tasks_queue;
typedef struct tasks_cursor { struct linux_list_list_head *at; } tasks_cursor;

#endif /* TASKS_TYPE_H */
