/******************************************************************************
 * fly_sched.c
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

#include <libfly/fly_debugging.h>
#include "fly_sched.h"
#include "fly_globals.h"
#include "fly_thread.h"
#include "fly_worker.h"
#include "fly_job.h"
#include "fly_task.h"
#include "fly_atomic.h"

/******************************************************************************
 * Helper functions declarations
 *****************************************************************************/
#define FLY_SCHED_ONE_LOCKS		1
#define FLY_SCHED_TWO_LOCKS		2
#define FLY_SCHED_TREE_LOCKS	3
#define FLY_SCHED_MAX_RLOCK		256
static inline int fly_sched_init_locks();
static inline void fly_sched_uninit_locks(int nb);

void fly_set_nbworkers(int nb)
{
	fly_sched.nbworkers = nb;
}

#define FLY_SCHED_UPDATE_INTERVAL_US	1000
static void *fly_sched_thread_func(void *param)
{
	while (fly_sched.thread->state == FLY_THREAD_RUNNING) {
		fly_thread_sleep(FLY_SCHED_UPDATE_INTERVAL_US);
	}
	return param;
}

/******************************************************************************
 * Initialization helpers.
 *****************************************************************************/

int fly_sched_init()
{
	int err;
	size_t tsize = sizeof(struct fly_worker) * fly_sched.nbworkers;

	err = fly_sched_init_locks();
	if (!FLY_SUCCEEDED(err))
		return err;

	fly_sched.workers = fly_malloc(tsize);

	if (fly_sched.workers) {
		int i = 0;
		fly_sched.initialized = 0;

		for (; i < fly_sched.nbworkers; i++) {
			err = fly_worker_init(&fly_sched.workers[i]);
			if (!FLY_SUCCEEDED(err))
				break;
			err = fly_worker_start(&fly_sched.workers[i]);
			if (!FLY_SUCCEEDED(err))
				break;
		}

		if (i == fly_sched.nbworkers) {
			fly_list_init(&fly_sched.ready_jobs);
			fly_list_init(&fly_sched.running_jobs);
			fly_list_init(&fly_sched.done_jobs);

			/* Init scheduler thread */
			fly_sched.thread = fly_malloc(sizeof(struct fly_thread));
			if (!fly_sched.thread) {
				fly_sched_uninit_locks(FLY_SCHED_TREE_LOCKS);
				for (i = 0; i < fly_sched.nbworkers; i++)
					fly_worker_uninit(&fly_sched.workers[i]);
				return FLYENORES;
			}
			err = fly_thread_init(fly_sched.thread, fly_sched_thread_func, &fly_sched);
			if (!FLY_SUCCEEDED(err)) {
				fly_free(fly_sched.thread);
				fly_sched_uninit_locks(FLY_SCHED_TREE_LOCKS);
				for (i = 0; i < fly_sched.nbworkers; i++)
					fly_worker_uninit(&fly_sched.workers[i]);
				return err;
			}
			err = fly_thread_start(fly_sched.thread);
			if (!FLY_SUCCEEDED(err)) {
				fly_thread_uninit(fly_sched.thread);
				fly_free(fly_sched.thread);
				fly_sched_uninit_locks(FLY_SCHED_TREE_LOCKS);
				for (i = 0; i < fly_sched.nbworkers; i++)
					fly_worker_uninit(&fly_sched.workers[i]);
				return err;
			}

			/* Finish initialization */
			fly_sched.initialized = 1;
		} else {
			fly_sched_uninit_locks(FLY_SCHED_TREE_LOCKS);
			for (; i >= 0; i--)
				fly_worker_uninit(&fly_sched.workers[i]);
		}

	} else {
		fly_sched_uninit_locks(FLY_SCHED_TREE_LOCKS);
		err = FLYENORES;
	}

	return err;
}

int fly_sched_uninit()
{
	int err = FLYESUCCESS;
	int i;

	/* Uninit scheduler thread */
	if (fly_sched.thread) {
		fly_sched.thread->state = FLY_THREAD_IDLE;
		fly_thread_wait(fly_sched.thread);
		fly_thread_uninit(fly_sched.thread);
		fly_free(fly_sched.thread);
	}

	for (i = 0; i < fly_sched.nbworkers; i++)
		fly_worker_request_exit(&fly_sched.workers[i]);

	for (i = 0; i < fly_sched.nbworkers; i++)
		fly_worker_uninit(&fly_sched.workers[i]);

	fly_free(fly_sched.workers);

	while (!fly_list_is_empty(&fly_sched.ready_jobs)) {
		void *el = fly_list_tail(&fly_sched.ready_jobs)->el;
		fly_list_remove(&fly_sched.ready_jobs, el);
	}
	while (!fly_list_is_empty(&fly_sched.running_jobs)) {
		void *el = fly_list_tail(&fly_sched.running_jobs)->el;
		fly_list_remove(&fly_sched.running_jobs, el);
	}
	while (!fly_list_is_empty(&fly_sched.done_jobs)) {
		void *el = fly_list_tail(&fly_sched.done_jobs)->el;
		fly_list_remove(&fly_sched.done_jobs, el);
	}

	fly_sched_uninit_locks(FLY_SCHED_TREE_LOCKS);

	return err;
}

/******************************************************************************
 * Add job implementations
 *****************************************************************************/
static inline int fly_sched_add_to_ready(struct fly_job *job)
{
	int err;
	fly_mrswlock_notrack_wlock(&fly_sched.ready_lock);
	err = fly_list_append(&fly_sched.ready_jobs, job);
	fly_mrswlock_wunlock(&fly_sched.ready_lock);
	return err;
}

static int fly_sched_add_pfj(struct fly_job *job)
{
	int err = fly_make_batches(job, fly_sched.nbworkers);
	if (FLY_SUCCEEDED(err)) {
		err = fly_sched_add_to_ready(job);
		if (!FLY_SUCCEEDED(err)) {
			fly_destroy_batches(job);
			return err;
		}
	}
	return err;
}

int fly_sched_add_job(struct fly_job *job)
{
	int err = FLYENOIMP;
	if ((job->jtype == FLY_TASK_PARALLEL_FOR) ||
			(job->jtype == FLY_TASK_PARALLEL_FOR_ARR)) {
		err = fly_sched_add_pfj(job);
	} else if(job->jtype == FLY_TASK_TASK) {
		err = fly_sched_add_to_ready(job);
	}
	if (FLY_SUCCEEDED(err)) {
		int i;
		for (i = 0; i < fly_sched.nbworkers; i++)
			fly_worker_work_available(&fly_sched.workers[i]);
	}
	return err;
}

int fly_sched_job_collected(struct fly_job *job)
{
	fly_mrswlock_notrack_wlock(&fly_sched.done_lock);
	fly_list_remove(&fly_sched.done_jobs, job);
	fly_mrswlock_wunlock(&fly_sched.done_lock);
	return FLYESUCCESS;
}

static int fly_pfarrj_exec(struct fly_job *job)
{
	struct fly_job_batch *batch = fly_get_exec_batch(job);
	if (batch) {
		int i;
		int count = batch->end - batch->start;
		char *param = batch->job->data + (batch->start * batch->job->elsize);

		for (i = 0; i < count; i++) {
			job->func.pfor(batch->start + i, param);
			param += batch->job->elsize;
		}
		fly_atomic_inc(&job->batches_done, 1);
		return 0;
	}
	return 1;
}

static int fly_pfptrj_exec(struct fly_job *job)
{
	struct fly_job_batch *batch = fly_get_exec_batch(job);
	if (batch) {
		int i;
		int count = batch->end - batch->start;
		for (i = 0; i < count; i++) {
			job->func.pfor(batch->start + i, job->data);
		}
		fly_atomic_inc(&job->batches_done, 1);
		return 0;
	}
	return 1;
}

static int fly_taskjob_exec(struct fly_job *job)
{
	struct fly_task *task = job->data;
	task->result = task->func(task->param);
	return 0;
}

static struct fly_job *fly_sched_get_ready(struct fly_thread *t)
{
	struct fly_job *job;
	fly_mrswlock_wlock(&fly_sched.ready_lock, t);
	job = fly_list_tail_remove(&fly_sched.ready_jobs);
	fly_mrswlock_wunlock(&fly_sched.ready_lock);
	return job;
}

static struct fly_job *fly_sched_get_running(struct fly_thread *t)
{
	struct fly_job *job = NULL;
	struct fly_list *curr;
	fly_mrswlock_rlock(&fly_sched.running_lock, t);
	curr = fly_list_head(&fly_sched.running_jobs);
	while (curr) {
		job = curr->el;
		if (job->batches_done == job->nbbatches) {
			curr = curr->next;
			continue;
		}
		else
			break;
	}
	fly_mrswlock_runlock(&fly_sched.running_lock);
	return job;
}

static void fly_sched_move_to_running(struct fly_job *job, struct fly_thread *t)
{
	fly_mrswlock_wlock(&fly_sched.running_lock, t);
	job->state = FLY_JOB_RUNNING;
	fly_list_append(&fly_sched.running_jobs, job);
	fly_mrswlock_wunlock(&fly_sched.running_lock);
}

static void fly_sched_remove_running(struct fly_job *job)
{
	fly_mrswlock_notrack_wlock(&fly_sched.running_lock);
	fly_list_remove(&fly_sched.running_jobs, job);
	fly_mrswlock_wunlock(&fly_sched.running_lock);
}

static void fly_sched_move_to_done(struct fly_job *job)
{
	fly_mrswlock_notrack_wlock(&fly_sched.done_lock);
	fly_list_append(&fly_sched.done_jobs, job);
	fly_mrswlock_wunlock(&fly_sched.done_lock);
}

static struct fly_job *fly_sched_get_job(struct fly_worker_thread *wthread)
{
	struct fly_job *job = fly_sched_get_ready(&wthread->thread);
	if (job) {
		if ((job->jtype == FLY_TASK_PARALLEL_FOR) ||
				(job->jtype == FLY_TASK_PARALLEL_FOR_ARR)) {
			int i;
			fly_sched_move_to_running(job, &wthread->thread);
			for (i = 0; i < fly_sched.nbworkers; i++) {
				if (&fly_sched.workers[i] != wthread->parent)
					fly_worker_work_available(&fly_sched.workers[i]);
			}
		} else if (job->jtype == FLY_TASK_TASK) {
			/* prevents other threads to get this job as running */
			job->batches_done = job->nbbatches;
			fly_sched_move_to_running(job, &wthread->thread);
		} else {
			fly_assert(0, "fly_schedule unsupported job type");
		}
	} else {
		job = fly_sched_get_running(&wthread->thread);
	}
	return job;
}

static int fly_sched_exec_job(struct fly_job *job)
{
	int shouldsleep;

	int nbbatches = job->nbbatches;
	if (job->jtype == FLY_TASK_PARALLEL_FOR) {
		shouldsleep = fly_pfptrj_exec(job);
	} else if (job->jtype == FLY_TASK_PARALLEL_FOR_ARR) {
		shouldsleep = fly_pfarrj_exec(job);
	} else if (job->jtype == FLY_TASK_TASK) {
		shouldsleep = fly_taskjob_exec(job);
	} else {
		fly_assert(0, "[fly_sched] unsupported job type");
		shouldsleep = 1;
	}

	if (fly_atomic_cas(&job->batches_done, nbbatches, nbbatches)) {
		job->state = FLY_JOB_DONE;
		fly_sched_remove_running(job);
		fly_sched_move_to_done(job);
		fly_sem_post(&job->sem);
	}
	return shouldsleep;
}

void fly_schedule(struct fly_worker_thread *wthread)
{
	struct fly_job *job = fly_sched_get_job(wthread);
	if (!job || (fly_sched_exec_job(job) != 0))
		fly_worker_thread_wait_work(wthread);
}

/******************************************************************************
 * Helper functions implementations
 *****************************************************************************/
static inline int fly_sched_init_locks()
{
	int err;
	err = fly_mrswlock_init(&fly_sched.ready_lock, FLY_SCHED_MAX_RLOCK);
	if (err == 0) {
		err = fly_mrswlock_init(&fly_sched.running_lock, FLY_SCHED_MAX_RLOCK);
		if (err == 0) {
			err = fly_mrswlock_init(&fly_sched.done_lock,
					FLY_SCHED_MAX_RLOCK);
			if (err != 0) {
				fly_sched_uninit_locks(FLY_SCHED_TWO_LOCKS);
				err = FLYENORES;
			}
		} else {
			fly_sched_uninit_locks(FLY_SCHED_ONE_LOCKS);
			err = FLYENORES;
		}
	}
	return err;
}

static inline void fly_sched_uninit_locks(int nb)
{
	if (nb >= FLY_SCHED_TREE_LOCKS)
		fly_mrswlock_uninit(&fly_sched.done_lock);
	if (nb >= FLY_SCHED_TWO_LOCKS)
		fly_mrswlock_uninit(&fly_sched.running_lock);
	if (nb >= FLY_SCHED_ONE_LOCKS)
		fly_mrswlock_uninit(&fly_sched.ready_lock);
}
