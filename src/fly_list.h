/******************************************************************************
 * fly_list.h
 *
 * Copyright (C) 2013 Kostadin Atanasov <pranayama111@gmail.com>
 *
 * This file is part of libfly.
 *
 * libfly is free software: you can redistribute it and/or modify
 * if under the terms of GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * libfly is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should received a copy of the GNU Lesser General Public License
 * along with libfly. If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************/

#ifndef FLY_LIST_H
#define FLY_LIST_H

#include "fly_globals.h"

#include <stdlib.h>

struct fly_list {
	struct fly_list	*next;
	struct fly_list *prev;
	void			*el;
}; /* struct fly_list */

#define FLY_LIST_ELEMENT(list, elType) (elType)list->el

static inline void fly_list_init(struct fly_list *list)
{
	list->next = NULL;
	list->prev = NULL;
	list->el = NULL;
}

static inline void fly_list_init_with_el(struct fly_list *list, void *el)
{
	list->next = NULL;
	list->prev = NULL;
	list->el = el;
}

static inline struct fly_list *fly_list_head(struct fly_list *list)
{
	return list->next;
}

static inline struct fly_list *fly_list_tail(struct fly_list *list)
{
	return list->prev;
}

static inline void fly_list_append_node(struct fly_list *list,
		struct fly_list *node)
{
	if (list->prev) {
		list->prev->next = node;
		node->prev = list->prev;
	} else {
		list->next = node;
	}
	list->prev = node;
}

static inline int fly_list_append(struct fly_list *list, void *el)
{
	struct fly_list *node = fly_malloc(sizeof(struct fly_list));
	if (node) {
		fly_list_init_with_el(node, el);
		fly_list_append_node(list, node);
		return FLYESUCCESS;
	}
	return FLYENORES;
}

static inline void fly_list_prepend_node(struct fly_list *list,
		struct fly_list *node)
{
	node->next = list->next;
	list->next = node;
}

static inline int fly_list_prepend(struct fly_list *list, void *el)
{
	struct fly_list *node = fly_malloc(sizeof(struct fly_list));
	if (node) {
		fly_list_init_with_el(node, el);
		fly_list_prepend_node(list, node);
		return FLYESUCCESS;
	}
	return FLYENORES;
}

static inline void fly_list_remove_node(struct fly_list *list,
		struct fly_list *node)
{
	struct fly_list *head = fly_list_head(list);
	if (list->next == node)
		list->next = node->next;
	if (list->prev == node)
		list->prev = node->prev;

	while (head && (head != node)) {
		head = head->next;
	}
	if (head) {
		if (head->prev)
			head->prev->next = head->next;
		if (head->next)
			head->next->prev = head->prev;
	}
}

static inline struct fly_list *fly_list_get(struct fly_list *list, void *el)
{
	struct fly_list *node = list->next;
	while (node && (node->el != el))
		node = node->next;
	return node;
}

static inline void fly_list_remove(struct fly_list *list, void *el)
{
	struct fly_list *node = fly_list_get(list, el);
	if (node) {
		fly_list_remove_node(list, node);
		fly_free(node);
	}
}

static inline int fly_list_is_empty(struct fly_list *list)
{
	return list->next == NULL;
}

static inline void *fly_list_tail_remove(struct fly_list *list)
{
	void *ret;
	struct fly_list *tail = fly_list_tail(list);
	if (tail) {
		ret = tail->el;
		fly_list_remove_node(list, tail);
		fly_free(tail);
	} else {
		ret = NULL;
	}
	return ret;
}

#endif /* FLY_LIST_H */
