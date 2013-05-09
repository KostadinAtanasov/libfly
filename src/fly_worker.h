/******************************************************************************
 * fly_worker.h
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

#ifndef FLY_WORKER_H
#define FLY_WORKER_H

#include "fly_list.h"
#include "fly_thread.h"
#include "fly_sem.h"

typedef int fly_worker_state;
#define FLY_WORKER_IDLE		0
#define FLY_WORKER_RUNNING	1
#define FLY_WORKER_EXITING	2
#define FLY_WORKER_FINISHED	3

#define FLY_WORKER_NB_THREADS	2

struct fly_worker_thread {
	struct fly_thread	thread;
	struct fly_sem		sem;
	struct fly_worker	*parent;
	fly_worker_state	tstate;
	int					active;
}; /* struct fly_worker_thread */

struct fly_worker {
	struct fly_worker_thread	mainthread;
	struct fly_worker_thread	backupthread;
	int							mainblocked;
}; /* fly_worker */

/******************************************************************************
 * fly_worker interface
 *****************************************************************************/
int fly_worker_init(struct fly_worker *worker);
int fly_worker_uninit(struct fly_worker *worker);
int fly_worker_start(struct fly_worker *worker);
void fly_worker_request_exit(struct fly_worker *worker);
int fly_worker_wait(struct fly_worker *worker);
void fly_worker_work_available(struct fly_worker *worker);

/******************************************************************************
 * fly_worker_thread interface
 *****************************************************************************/
void fly_worker_thread_wait_work(struct fly_worker_thread *thread);

#endif /* FLY_WORKER_H */
