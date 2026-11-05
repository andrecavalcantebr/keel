/* linux/linux_list.h — generated from linux/list.k by cgen, C23 profile. */
#ifndef LINUX_LINUX_LIST_H
#define LINUX_LINUX_LIST_H

#include "linux/linux_list.type.h"

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
#line 634 "linux/list.k"
static inline void linux_list_list_del_range(struct linux_list_list_head *begin,
				  struct linux_list_list_head *end);
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
