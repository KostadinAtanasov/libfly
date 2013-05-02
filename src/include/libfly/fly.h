/******************************************************************************
 * fly.h
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

#ifndef LIBFLY_FLY_H
#define LIBFLY_FLY_H

#include <libfly/fly_error.h>
#include <libfly/fly_debugging.h>

#include <stddef.h>

/******************************************************************************
 * libfly memory management
 *****************************************************************************/
typedef void*(*fly_malloc_func)(size_t);
typedef void (*fly_free_func)(void*);
void fly_set_malloc(fly_malloc_func func);
void fly_set_free(fly_free_func func);

/******************************************************************************
 * libfly initialization
 *****************************************************************************/
int fly_simple_init(int nbt);
int fly_uninit();


typedef void (*fly_parallel_for_func)(int, void*);
int fly_parallel_for(int count, fly_parallel_for_func func, void *ptr);
int fly_parallel_for_arr(int start, int end, fly_parallel_for_func func,
		void *arr, size_t elsize);

/******************************************************************************
 * Task parallelism
 *****************************************************************************/
typedef void *(*fly_task_func)(void*);
struct fly_task;

struct fly_task *fly_create_task(fly_task_func func, void *param);
void fly_destroy_task(struct fly_task *task);
int fly_push_task(struct fly_task *task);
int fly_wait_task(struct fly_task *task);
int fly_wait_tasks(struct fly_task **tasks, int nbtasks);

#endif /* LIBFLY_FLY_H */
