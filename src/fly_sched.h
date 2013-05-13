/******************************************************************************
 * fly_sched.h
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

#ifndef FLY_SCHEDULER_H
#define FLY_SCHEDULER_H

#include <libfly/fly_error.h>

#include "fly_mrswlock.h"
#include "fly_list.h"

/* Forwards */
struct fly_worker;
struct fly_worker_thread;
struct fly_job;
struct fly_thread;

/*****************************************************************************
 * Supported job/task types
 ****************************************************************************/
#define FLY_TASK_PARALLEL_FOR		1
#define FLY_TASK_PARALLEL_FOR_ARR	2
#define FLY_TASK_TASK				3

#define FLY_SCHED_THREAD_IDLE		0
#define FLY_SCHED_THREAD_RUNNING	1
#define FLY_SCHED_THREAD_STOPPING	2

struct fly_sched {
	struct fly_worker		*workers;
	int						nbworkers;
	int						initialized;

	struct fly_list			ready_jobs;
	struct fly_list			running_jobs;
	struct fly_list			done_jobs;

	struct fly_mrswlock		ready_lock;
	struct fly_mrswlock		running_lock;
	struct fly_mrswlock		done_lock;

	struct fly_thread		*thread;
	int						threadstate;
}; /* fly_sched */

void fly_set_nbworkers(int nb);
int fly_get_nbworkers();

int fly_sched_init();
int fly_sched_uninit();

int fly_sched_add_job(struct fly_job *job);
int fly_sched_job_collected(struct fly_job *job);

void fly_schedule(struct fly_worker_thread *wthread);
void fly_sched_update();

#endif /* FLY_SCHEDULER_H */
