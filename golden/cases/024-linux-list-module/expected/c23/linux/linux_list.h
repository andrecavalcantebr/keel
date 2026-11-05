/* linux/linux_list.h — generated from linux/list.k by cgen, C23 profile. */
#ifndef LINUX_LINUX_LIST_H
#define LINUX_LINUX_LIST_H

#include "linux/linux_list.type.h"

#line 19 "linux/list.k"
#ifndef __TOOLS_LINUX_LIST_H
#define __TOOLS_LINUX_LIST_H

#include <stddef.h>
#line 28 "linux/list.k"
#define LIST_POISON1  ((void *) 0x100)
#define LIST_POISON2  ((void *) 0x200)

#ifndef container_of
#define container_of(ptr, type, member) ({			\
	const typeof(((type *)0)->member) * __mptr = (ptr);	\
	(type *)((char *)__mptr - offsetof(type, member)); })
#endif

#define WRITE_ONCE(x, val) (*(volatile typeof(x) *)&(x) = (val))

/*
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)
#line 54 "linux/list.k"
static inline void linux_list_INIT_LIST_HEAD(struct linux_list_list_head *list);
#line 66 "linux/list.k"
static inline void linux_list___list_add(struct linux_list_list_head *new,
			      struct linux_list_list_head *prev,
			      struct linux_list_list_head *next);
#line 84 "linux/list.k"
static inline void linux_list_list_add(struct linux_list_list_head *new, struct linux_list_list_head *head);
#line 98 "linux/list.k"
static inline void linux_list_list_add_tail(struct linux_list_list_head *new, struct linux_list_list_head *head);
#line 110 "linux/list.k"
static inline void linux_list___list_del(struct linux_list_list_head * prev, struct linux_list_list_head * next);
#line 122 "linux/list.k"
static inline void linux_list___list_del_entry(struct linux_list_list_head *entry);
#line 127 "linux/list.k"
static inline void linux_list_list_del(struct linux_list_list_head *entry);
#line 141 "linux/list.k"
static inline void linux_list_list_replace(struct linux_list_list_head *old,
				struct linux_list_list_head *new);
#line 150 "linux/list.k"
static inline void linux_list_list_replace_init(struct linux_list_list_head *old,
					struct linux_list_list_head *new);
#line 161 "linux/list.k"
static inline void linux_list_list_del_init(struct linux_list_list_head *entry);
#line 172 "linux/list.k"
static inline void linux_list_list_move(struct linux_list_list_head *list, struct linux_list_list_head *head);
#line 183 "linux/list.k"
static inline void linux_list_list_move_tail(struct linux_list_list_head *list,
				  struct linux_list_list_head *head);
#line 195 "linux/list.k"
static inline int linux_list_list_is_first(const struct linux_list_list_head *list, const struct linux_list_list_head *head);
#line 205 "linux/list.k"
static inline int linux_list_list_is_last(const struct linux_list_list_head *list,
				const struct linux_list_list_head *head);
#line 215 "linux/list.k"
static inline int linux_list_list_empty(const struct linux_list_list_head *head);
#line 233 "linux/list.k"
static inline int linux_list_list_empty_careful(const struct linux_list_list_head *head);
#line 243 "linux/list.k"
static inline void linux_list_list_rotate_left(struct linux_list_list_head *head);
#line 257 "linux/list.k"
static inline int linux_list_list_is_singular(const struct linux_list_list_head *head);
#line 262 "linux/list.k"
static inline void linux_list___list_cut_position(struct linux_list_list_head *list,
		struct linux_list_list_head *head, struct linux_list_list_head *entry);
#line 288 "linux/list.k"
static inline void linux_list_list_cut_position(struct linux_list_list_head *list,
		struct linux_list_list_head *head, struct linux_list_list_head *entry);
#line 302 "linux/list.k"
static inline void linux_list___list_splice(const struct linux_list_list_head *list,
				 struct linux_list_list_head *prev,
				 struct linux_list_list_head *next);
#line 321 "linux/list.k"
static inline void linux_list_list_splice(const struct linux_list_list_head *list,
				struct linux_list_list_head *head);
#line 333 "linux/list.k"
static inline void linux_list_list_splice_tail(struct linux_list_list_head *list,
				struct linux_list_list_head *head);
#line 347 "linux/list.k"
static inline void linux_list_list_splice_init(struct linux_list_list_head *list,
				    struct linux_list_list_head *head);
#line 364 "linux/list.k"
static inline void linux_list_list_splice_tail_init(struct linux_list_list_head *list,
					 struct linux_list_list_head *head);
#line 379 "linux/list.k"
#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

/**
 * list_first_entry - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_head within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)

/**
 * list_last_entry - get the last element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_head within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define list_last_entry(ptr, type, member) \
	list_entry((ptr)->prev, type, member)

/**
 * list_first_entry_or_null - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_head within the struct.
 *
 * Note that if the list is empty, it returns NULL.
 */
#define list_first_entry_or_null(ptr, type, member) \
	(!list_empty(ptr) ? list_first_entry(ptr, type, member) : NULL)

/**
 * list_last_entry_or_null - get the last element from a list
 * @ptr:       the list head to take the element from.
 * @type:      the type of the struct this is embedded in.
 * @member:    the name of the list_head within the struct.
 *
 * Note that if the list is empty, it returns NULL.
 */
#define list_last_entry_or_null(ptr, type, member) \
	(!list_empty(ptr) ? list_last_entry(ptr, type, member) : NULL)

/**
 * list_next_entry - get the next element in list
 * @pos:	the type * to cursor
 * @member:	the name of the list_head within the struct.
 */
#define list_next_entry(pos, member) \
	list_entry((pos)->member.next, typeof(*(pos)), member)

/**
 * list_prev_entry - get the prev element in list
 * @pos:	the type * to cursor
 * @member:	the name of the list_head within the struct.
 */
#define list_prev_entry(pos, member) \
	list_entry((pos)->member.prev, typeof(*(pos)), member)

/**
 * list_for_each	-	iterate over a list
 * @pos:	the &struct list_head to use as a loop cursor.
 * @head:	the head for your list.
 */
#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * list_for_each_prev	-	iterate over a list backwards
 * @pos:	the &struct list_head to use as a loop cursor.
 * @head:	the head for your list.
 */
#define list_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

/**
 * list_for_each_safe - iterate over a list safe against removal of list entry
 * @pos:	the &struct list_head to use as a loop cursor.
 * @n:		another &struct list_head to use as temporary storage
 * @head:	the head for your list.
 */
#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

/**
 * list_for_each_prev_safe - iterate over a list backwards safe against removal of list entry
 * @pos:	the &struct list_head to use as a loop cursor.
 * @n:		another &struct list_head to use as temporary storage
 * @head:	the head for your list.
 */
#define list_for_each_prev_safe(pos, n, head) \
	for (pos = (head)->prev, n = pos->prev; \
	     pos != (head); \
	     pos = n, n = pos->prev)

/**
 * list_for_each_entry	-	iterate over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_head within the struct.
 */
#define list_for_each_entry(pos, head, member)				\
	for (pos = list_first_entry(head, typeof(*pos), member);	\
	     &pos->member != (head);					\
	     pos = list_next_entry(pos, member))

/**
 * list_for_each_entry_reverse - iterate backwards over list of given type.
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_head within the struct.
 */
#define list_for_each_entry_reverse(pos, head, member)			\
	for (pos = list_last_entry(head, typeof(*pos), member);		\
	     &pos->member != (head); 					\
	     pos = list_prev_entry(pos, member))

/**
 * list_prepare_entry - prepare a pos entry for use in list_for_each_entry_continue()
 * @pos:	the type * to use as a start point
 * @head:	the head of the list
 * @member:	the name of the list_head within the struct.
 *
 * Prepares a pos entry for use as a start point in list_for_each_entry_continue().
 */
#define list_prepare_entry(pos, head, member) \
	((pos) ? : list_entry(head, typeof(*pos), member))

/**
 * list_for_each_entry_continue - continue iteration over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_head within the struct.
 *
 * Continue to iterate over list of given type, continuing after
 * the current position.
 */
#define list_for_each_entry_continue(pos, head, member) 		\
	for (pos = list_next_entry(pos, member);			\
	     &pos->member != (head);					\
	     pos = list_next_entry(pos, member))

/**
 * list_for_each_entry_continue_reverse - iterate backwards from the given point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_head within the struct.
 *
 * Start to iterate over list of given type backwards, continuing after
 * the current position.
 */
#define list_for_each_entry_continue_reverse(pos, head, member)		\
	for (pos = list_prev_entry(pos, member);			\
	     &pos->member != (head);					\
	     pos = list_prev_entry(pos, member))

/**
 * list_for_each_entry_from - iterate over list of given type from the current point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_head within the struct.
 *
 * Iterate over list of given type, continuing from current position.
 */
#define list_for_each_entry_from(pos, head, member) 			\
	for (; &pos->member != (head);					\
	     pos = list_next_entry(pos, member))

/**
 * list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_head within the struct.
 */
#define list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = list_first_entry(head, typeof(*pos), member),	\
		n = list_next_entry(pos, member);			\
	     &pos->member != (head); 					\
	     pos = n, n = list_next_entry(n, member))

/**
 * list_for_each_entry_safe_continue - continue list iteration safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_head within the struct.
 *
 * Iterate over list of given type, continuing after current point,
 * safe against removal of list entry.
 */
#define list_for_each_entry_safe_continue(pos, n, head, member) 		\
	for (pos = list_next_entry(pos, member), 				\
		n = list_next_entry(pos, member);				\
	     &pos->member != (head);						\
	     pos = n, n = list_next_entry(n, member))

/**
 * list_for_each_entry_safe_from - iterate over list from current point safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_head within the struct.
 *
 * Iterate over list of given type from current point, safe against
 * removal of list entry.
 */
#define list_for_each_entry_safe_from(pos, n, head, member) 			\
	for (n = list_next_entry(pos, member);					\
	     &pos->member != (head);						\
	     pos = n, n = list_next_entry(n, member))

/**
 * list_for_each_entry_safe_reverse - iterate backwards over list safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_head within the struct.
 *
 * Iterate backwards over list of given type, safe against removal
 * of list entry.
 */
#define list_for_each_entry_safe_reverse(pos, n, head, member)		\
	for (pos = list_last_entry(head, typeof(*pos), member),		\
		n = list_prev_entry(pos, member);			\
	     &pos->member != (head); 					\
	     pos = n, n = list_prev_entry(n, member))

/**
 * list_safe_reset_next - reset a stale list_for_each_entry_safe loop
 * @pos:	the loop cursor used in the list_for_each_entry_safe loop
 * @n:		temporary storage used in list_for_each_entry_safe
 * @member:	the name of the list_head within the struct.
 *
 * list_safe_reset_next is not safe to use in general if the list may be
 * modified concurrently (eg. the lock is dropped in the loop body). An
 * exception to this is if the cursor element (pos) is pinned in the list,
 * and list_safe_reset_next is called after re-taking the lock and before
 * completing the current iteration of the loop body.
 */
#define list_safe_reset_next(pos, n, member)				\
	n = list_next_entry(pos, member)
#line 634 "linux/list.k"
static inline void linux_list_list_del_range(struct linux_list_list_head *begin,
				  struct linux_list_list_head *end);
#line 646 "linux/list.k"
#define list_for_each_from(pos, head) \
	for (; pos != (head); pos = pos->next)

#endif /* __TOOLS_LINUX_LIST_H */

#line 54 "linux/list.k"
static inline void linux_list_INIT_LIST_HEAD(struct linux_list_list_head *list)
{
	list->next = list;
	list->prev = list;
}
#line 66 "linux/list.k"
static inline void linux_list___list_add(struct linux_list_list_head *new,
			      struct linux_list_list_head *prev,
			      struct linux_list_list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}
#line 84 "linux/list.k"
static inline void linux_list_list_add(struct linux_list_list_head *new, struct linux_list_list_head *head)
{
	linux_list___list_add(new, head, head->next);
}
#line 98 "linux/list.k"
static inline void linux_list_list_add_tail(struct linux_list_list_head *new, struct linux_list_list_head *head)
{
	linux_list___list_add(new, head->prev, head);
}
#line 110 "linux/list.k"
static inline void linux_list___list_del(struct linux_list_list_head * prev, struct linux_list_list_head * next)
{
	next->prev = prev;
	WRITE_ONCE(prev->next, next);
}
#line 122 "linux/list.k"
static inline void linux_list___list_del_entry(struct linux_list_list_head *entry)
{
	linux_list___list_del(entry->prev, entry->next);
}
#line 127 "linux/list.k"
static inline void linux_list_list_del(struct linux_list_list_head *entry)
{
	linux_list___list_del(entry->prev, entry->next);
	entry->next = LIST_POISON1;
	entry->prev = LIST_POISON2;
}
#line 141 "linux/list.k"
static inline void linux_list_list_replace(struct linux_list_list_head *old,
				struct linux_list_list_head *new)
{
	new->next = old->next;
	new->next->prev = new;
	new->prev = old->prev;
	new->prev->next = new;
}
#line 150 "linux/list.k"
static inline void linux_list_list_replace_init(struct linux_list_list_head *old,
					struct linux_list_list_head *new)
{
	linux_list_list_replace(old, new);
	linux_list_INIT_LIST_HEAD(old);
}
#line 161 "linux/list.k"
static inline void linux_list_list_del_init(struct linux_list_list_head *entry)
{
	linux_list___list_del_entry(entry);
	linux_list_INIT_LIST_HEAD(entry);
}
#line 172 "linux/list.k"
static inline void linux_list_list_move(struct linux_list_list_head *list, struct linux_list_list_head *head)
{
	linux_list___list_del_entry(list);
	linux_list_list_add(list, head);
}
#line 183 "linux/list.k"
static inline void linux_list_list_move_tail(struct linux_list_list_head *list,
				  struct linux_list_list_head *head)
{
	linux_list___list_del_entry(list);
	linux_list_list_add_tail(list, head);
}
#line 195 "linux/list.k"
static inline int linux_list_list_is_first(const struct linux_list_list_head *list, const struct linux_list_list_head *head)
{
	return list->prev == head;
}
#line 205 "linux/list.k"
static inline int linux_list_list_is_last(const struct linux_list_list_head *list,
				const struct linux_list_list_head *head)
{
	return list->next == head;
}
#line 215 "linux/list.k"
static inline int linux_list_list_empty(const struct linux_list_list_head *head)
{
	return head->next == head;
}
#line 233 "linux/list.k"
static inline int linux_list_list_empty_careful(const struct linux_list_list_head *head)
{
	struct linux_list_list_head *next = head->next;
	return (next == head) && (next == head->prev);
}
#line 243 "linux/list.k"
static inline void linux_list_list_rotate_left(struct linux_list_list_head *head)
{
	struct linux_list_list_head *first;

	if (!linux_list_list_empty(head)) {
		first = head->next;
		linux_list_list_move_tail(first, head);
	}
}
#line 257 "linux/list.k"
static inline int linux_list_list_is_singular(const struct linux_list_list_head *head)
{
	return !linux_list_list_empty(head) && (head->next == head->prev);
}
#line 262 "linux/list.k"
static inline void linux_list___list_cut_position(struct linux_list_list_head *list,
		struct linux_list_list_head *head, struct linux_list_list_head *entry)
{
	struct linux_list_list_head *new_first = entry->next;
	list->next = head->next;
	list->next->prev = list;
	list->prev = entry;
	entry->next = list;
	head->next = new_first;
	new_first->prev = head;
}
#line 288 "linux/list.k"
static inline void linux_list_list_cut_position(struct linux_list_list_head *list,
		struct linux_list_list_head *head, struct linux_list_list_head *entry)
{
	if (linux_list_list_empty(head))
		return;
	if (linux_list_list_is_singular(head) &&
		(head->next != entry && head != entry))
		return;
	if (entry == head)
		linux_list_INIT_LIST_HEAD(list);
	else
		linux_list___list_cut_position(list, head, entry);
}
#line 302 "linux/list.k"
static inline void linux_list___list_splice(const struct linux_list_list_head *list,
				 struct linux_list_list_head *prev,
				 struct linux_list_list_head *next)
{
	struct linux_list_list_head *first = list->next;
	struct linux_list_list_head *last = list->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}
#line 321 "linux/list.k"
static inline void linux_list_list_splice(const struct linux_list_list_head *list,
				struct linux_list_list_head *head)
{
	if (!linux_list_list_empty(list))
		linux_list___list_splice(list, head, head->next);
}
#line 333 "linux/list.k"
static inline void linux_list_list_splice_tail(struct linux_list_list_head *list,
				struct linux_list_list_head *head)
{
	if (!linux_list_list_empty(list))
		linux_list___list_splice(list, head->prev, head);
}
#line 347 "linux/list.k"
static inline void linux_list_list_splice_init(struct linux_list_list_head *list,
				    struct linux_list_list_head *head)
{
	if (!linux_list_list_empty(list)) {
		linux_list___list_splice(list, head, head->next);
		linux_list_INIT_LIST_HEAD(list);
	}
}
#line 364 "linux/list.k"
static inline void linux_list_list_splice_tail_init(struct linux_list_list_head *list,
					 struct linux_list_list_head *head)
{
	if (!linux_list_list_empty(list)) {
		linux_list___list_splice(list, head->prev, head);
		linux_list_INIT_LIST_HEAD(list);
	}
}
#line 634 "linux/list.k"
static inline void linux_list_list_del_range(struct linux_list_list_head *begin,
				  struct linux_list_list_head *end)
{
	begin->prev->next = end->next;
	end->next->prev = begin->prev;
}

#endif /* LINUX_LINUX_LIST_H */
