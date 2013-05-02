/******************************************************************************
 * fly.c
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

#include <libfly/fly.h>
#include "fly_sched.h"
#include "fly_job.h"
#include "fly_task.h"
#include "fly_memdebug.h"

#include <stdlib.h>

/* The one and only */
struct fly_sched fly_sched;
fly_malloc_func fly_malloc;
fly_free_func   fly_free;

/******************************************************************************
 * libfly memory management
 *****************************************************************************/
void fly_set_malloc(fly_malloc_func func)
{
	fly_malloc = func;
}

void fly_set_free(fly_free_func func)
{
	fly_free = func;
}

/******************************************************************************
 * Default implementation for initialization needed functions
 *****************************************************************************/
void* fly_default_malloc(size_t size)
{
	void *ptr = malloc(size);
	fly_mem_count_alloc(ptr, size);
	return ptr;
}

void fly_default_free(void *ptr)
{
	fly_mem_count_free(ptr);
	free(ptr);
}

int fly_simple_init(int nbw)
{
	fly_set_malloc(fly_default_malloc);
	fly_set_free(fly_default_free);
	fly_set_nbworkers(nbw);

	fly_init_memdebug();

	return fly_sched_init();
}

int fly_uninit()
{
	int err = fly_sched_uninit();

	fly_memdebug_report(printf);
	fly_uninit_memdebug();
	return err;
}

/******************************************************************************
 * Parallel for parallelism
 *****************************************************************************/
int fly_parallel_for(int count, fly_parallel_for_func func, void *ptr)
{
	int err = FLYENORES;
	struct fly_job *job = fly_create_job_pfor(count, func, ptr);
	if (job) {
		err = fly_sched_add_job(job);
		if (FLY_SUCCEEDED(err))
			err = fly_wait_job(job);
		fly_destroy_job(job);
	}
	return err;
}

int fly_parallel_for_arr(int start, int end, fly_parallel_for_func func,
		void *arr, size_t elsize)
{
	int err = FLYENORES;
	struct fly_job *job = fly_create_job_pfarr(start, end, func, arr, elsize);
	if (job) {
		err = fly_sched_add_job(job);
		if (FLY_SUCCEEDED(err))
			err = fly_wait_job(job);
		fly_destroy_job(job);
	}
	return err;
}

/******************************************************************************
 * Task parallelism
 *****************************************************************************/
struct fly_task *fly_create_task(fly_task_func func, void *param)
{
	struct fly_task *task = fly_malloc(sizeof(struct fly_task));
	task->func = func;
	task->param = param;
	task->sched_data = NULL;
	return task;
}

void fly_destroy_task(struct fly_task *task)
{
	fly_free(task);
}

int fly_push_task(struct fly_task *task)
{
	int err = FLYENORES;
	struct fly_job *job = fly_create_job_task(task);
	if (job) {
		task->sched_data = job;
		err = fly_sched_add_job(job);
		if (!FLY_SUCCEEDED(err)) {
			task->sched_data = NULL;
			fly_destroy_job(job);
		}
	}
	return err;
}

int fly_wait_task(struct fly_task *task)
{
	int err = FLYEATTR;
	if (task->sched_data) {
		struct fly_job *job = task->sched_data;
		err = fly_wait_job(job);
		if (FLY_SUCCEEDED(err))
			fly_destroy_job(job);
	}
	return err;
}

int fly_wait_tasks(struct fly_task **tasks, int nbtasks)
{
	int i;
	int err = FLYESUCCESS;
	for (i = 0; i < nbtasks; i++) {
		int tmp = fly_wait_task(tasks[i]);
		if (!FLY_SUCCEEDED(tmp))
			err = tmp;
	}
	return err;
}
