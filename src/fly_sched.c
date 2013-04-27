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
#include "fly_worker.h"
#include "fly_job.h"
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
static int fly_sched_add_pfj(struct fly_job *job)
{
	int err = fly_make_batches(job, fly_sched.nbworkers);
	if (FLY_SUCCEEDED(err)) {
		fly_mrswlock_wlock(&fly_sched.ready_lock);
		err = fly_list_append(&fly_sched.ready_jobs, job);
		fly_mrswlock_wunlock(&fly_sched.ready_lock);
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
	}
	if (FLY_SUCCEEDED(err)) {
		int i;
		for (i = 0; i < fly_sched.nbworkers; i++)
			sem_post(&fly_sched.workers[i].sem);
	}
	return err;
}

static void fly_pfarrj_exec(struct fly_job *job)
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
	}
}

static void fly_pfptrj_exec(struct fly_job *job)
{
	struct fly_job_batch *batch = fly_get_exec_batch(job);
	if (batch) {
		int i;
		int count = batch->end - batch->start;
		for (i = 0; i < count; i++) {
			job->func.pfor(batch->start + i, job->data);
		}
		fly_atomic_inc(&job->batches_done, 1);
	}
}

static struct fly_job *fly_sched_get_ready()
{
	struct fly_job *job;
	fly_mrswlock_wlock(&fly_sched.ready_lock);
	job = fly_list_tail_remove(&fly_sched.ready_jobs);
	fly_mrswlock_wunlock(&fly_sched.ready_lock);
	return job;
}

static struct fly_job *fly_sched_get_running()
{
	struct fly_job *job = NULL;
	struct fly_list *curr;
	fly_mrswlock_rlock(&fly_sched.running_lock);
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

static void fly_sched_move_to_running(struct fly_job *job)
{
	fly_mrswlock_wlock(&fly_sched.running_lock);
	job->state = FLY_JOB_RUNNING;
	fly_list_append(&fly_sched.running_jobs, job);
	fly_mrswlock_wunlock(&fly_sched.running_lock);
}

static void fly_sched_remove_running(struct fly_job *job)
{
	fly_mrswlock_wlock(&fly_sched.running_lock);
	fly_list_remove(&fly_sched.running_jobs, job);
	fly_mrswlock_wunlock(&fly_sched.running_lock);
}

static void fly_sched_move_to_done(struct fly_job *job)
{
	fly_mrswlock_wlock(&fly_sched.done_lock);
	fly_list_append(&fly_sched.done_jobs, job);
	fly_mrswlock_wunlock(&fly_sched.done_lock);
}

void fly_schedule(struct fly_worker *w)
{
	/* TODO:
	 * Locks are for very short time so consider spinning...
	 */
	struct fly_job *job = fly_sched_get_ready();

	if (job) {
		if ((job->jtype == FLY_TASK_PARALLEL_FOR) ||
				(job->jtype == FLY_TASK_PARALLEL_FOR_ARR)) {
			int i;
			fly_sched_move_to_running(job);
			for (i = 0; i < fly_sched.nbworkers; i++) {
				if (&fly_sched.workers[i] != w)
					sem_post(&fly_sched.workers[i].sem);
			}
		} else {
			fly_assert(0, "fly_schedule unsupported job type");
		}
	} else {
		job = fly_sched_get_running();
	}

	if (job) {
		int nbbatches = job->nbbatches;
		if (job->jtype == FLY_TASK_PARALLEL_FOR) {
			fly_pfptrj_exec(job);
		} else if (job->jtype == FLY_TASK_PARALLEL_FOR_ARR) {
			fly_pfarrj_exec(job);
		}

		if (fly_atomic_cas(&job->batches_done, nbbatches, nbbatches)) {
			job->state = FLY_JOB_DONE;
			fly_sched_remove_running(job);
			fly_sched_move_to_done(job);
			sem_post(&job->sem);
		}
		return;
	}

	sem_wait(&w->sem);
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