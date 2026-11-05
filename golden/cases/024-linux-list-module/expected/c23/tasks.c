/* tasks.c — generated from tasks.k by cgen, C23 profile. */
#include "tasks.h"
#include "linux/linux_list.h"
#line 1 "tasks.k"






















int tasks_main(void) {
    tasks_queue q;
    linux_list_INIT_LIST_HEAD(&q.head);


    tasks_task pool[5];
    for (i32 i = 0; i < 5; i++) {
        pool[i].id = i + 1;
        linux_list_list_add_tail(&pool[i].link, &q.head);
    }


    tasks_task *t, *n;
    list_for_each_entry_safe(t, n, &q.head, link) {
        if (t->id % 2 == 0) linux_list_list_del(&t->link);
    }


    i32 want[] = { 1, 3, 5 };
    size_t k = 0;
    {   tasks_queue *keel__c0 = &q; tasks_cursor c = tasks_begin(keel__c0); while (tasks_has_next(keel__c0, &c)) { tasks_task *p = tasks_next(keel__c0, &c);
        printf("%d\n", (int)p->id);
        assert(p->id == want[k++]);
    } }
    assert(k == 3);


    k = 0;
    list_for_each_entry(t, &q.head, link) {
        assert(t->id == want[k++]);
    }
    assert(k == 3);
    return 0;
}
