/******************************************************************************
 * fly_thread.c
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

#include <libfly/fly_error.h>
#include "fly_globals.h"
#include "fly_worker.h"
#include "fly_sched.h"
#include "fly_atomic.h"

#include <string.h> /* for memset */

#define FLY_WORKER_STATE_WAIT_NANOSEC 1000

/******************************************************************************
 * fly_worker_thread interface
 *****************************************************************************/
inline void fly_worker_thread_wait_work(struct fly_worker_thread *thread)
{
	fly_sem_wait(&thread->sem, &thread->thread);
}

/******************************************************************************
 * fly_worker_thread private interface
 *****************************************************************************/
void* fly_worker_thread_func(void *wthread)
{
	struct fly_worker_thread *wt = (struct fly_worker_thread*)wthread;
	while (wt->tstate != FLY_WORKER_RUNNING)
		fly_thread_sleep(FLY_WORKER_STATE_WAIT_NANOSEC);

	while (wt->tstate == FLY_WORKER_RUNNING) {
		fly_schedule(wt);
	}

	wt->tstate = FLY_WORKER_FINISHED;
	return wt;
}

static inline void fly_worker_thread_work_available(struct fly_worker_thread *t)
{
	fly_sem_post(&t->sem);
}

static inline int fly_worker_thread_init(struct fly_worker_thread *thread)
{
	int err = FLYESUCCESS;
	err = fly_thread_init(&thread->thread, fly_worker_thread_func, thread);
	if (!FLY_SUCCEEDED(err))
		return err;
	err = fly_sem_init(&thread->sem);
	if (!FLY_SUCCEEDED(err)) {
		fly_thread_uninit(&thread->thread);
	}
	thread->tstate = FLY_WORKER_IDLE;
	return err;
}

static inline void fly_worker_thread_uninit(struct fly_worker_thread *thread)
{
	fly_thread_uninit(&thread->thread);
	fly_sem_uninit(&thread->sem);
}

static inline int fly_worker_thread_start(struct fly_worker_thread *thread)
{
	int ret = fly_thread_start(&thread->thread);
	if (FLY_SUCCEEDED(ret))
		thread->tstate = FLY_WORKER_RUNNING;
	return ret;
}

static inline int fly_worker_thread_request_exit(struct fly_worker_thread *thread)
{
	thread->tstate = FLY_WORKER_EXITING;
	return fly_sem_post(&thread->sem);
}

static inline int fly_worker_thread_wait(struct fly_worker_thread *thread)
{
	return fly_thread_wait(&thread->thread);
}

/******************************************************************************
 * fly_worker interface
 *****************************************************************************/
int fly_worker_init(struct fly_worker *worker)
{
	int err = FLYESUCCESS;
	memset(worker, 0, sizeof(struct fly_worker));

	worker->mainblocked = 0;

	worker->mainthread.parent = worker;
	err = fly_worker_thread_init(&worker->mainthread);
	if (!FLY_SUCCEEDED(err))
		return err;

	worker->backupthread.parent = worker;
	err = fly_worker_thread_init(&worker->backupthread);
	if (!FLY_SUCCEEDED(err)) {
		fly_worker_thread_uninit(&worker->mainthread);
	}

	return err;
}

int fly_worker_uninit(struct fly_worker *worker)
{
	int err = FLYESUCCESS;
	fly_worker_request_exit(worker);
	err = fly_worker_wait(worker);
	if (FLY_SUCCEEDED(err)) {
		fly_worker_thread_uninit(&worker->mainthread);
		fly_worker_thread_uninit(&worker->backupthread);
	}
	return err;
}

int fly_worker_start(struct fly_worker *worker)
{
	int ret;
	ret = fly_worker_thread_start(&worker->mainthread);
	if (FLY_SUCCEEDED(ret)) {
		ret = fly_worker_thread_start(&worker->backupthread);
		if (!FLY_SUCCEEDED(ret)) {
		}
	}
	return ret;
}

void fly_worker_request_exit(struct fly_worker *worker)
{
	fly_worker_thread_request_exit(&worker->mainthread);
	fly_worker_thread_request_exit(&worker->backupthread);
}

int fly_worker_wait(struct fly_worker *worker)
{
	int err = fly_worker_thread_wait(&worker->mainthread);
	err |= fly_worker_thread_wait(&worker->backupthread);
	return err;
}

void fly_worker_work_available(struct fly_worker *worker)
{
	if (fly_atomic_and(&worker->mainblocked, 1))
		fly_worker_thread_work_available(&worker->backupthread);
	else
		fly_worker_thread_work_available(&worker->mainthread);
}
