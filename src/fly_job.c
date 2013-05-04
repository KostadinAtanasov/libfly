/******************************************************************************
 * fly_job.c
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

#include "fly_job.h"
#include "fly_globals.h"
#include "fly_sched.h"
#include "fly_task.h"
#include "fly_atomic.h"

/******************************************************************************
 * Helper functions declarations.
 *****************************************************************************/
static struct fly_job_batch *job_make_batch(struct fly_job *job,
		int start, int end);
static void job_destroy_batch(struct fly_job_batch *batch);
static int job_add_batch(struct fly_job *job, struct fly_job_batch *batch);
static void job_remove_batch(struct fly_job *job, struct fly_job_batch *batch);

struct fly_job *fly_create_job_pfor(int count, fly_parallel_for_func func,
		void *ptr)
{
	struct fly_job *job = fly_malloc(sizeof(struct fly_job));
	if (job) {
		if (fly_sem_init(&job->sem) == 0) {
			job->data = ptr;
			job->start = 0;
			job->end = count;
			job->state = FLY_JOB_IDLE;
			fly_list_init(&job->batches);
			job->nbbatches = 0;
			job->batches_done = 0;
			job->func.pfor = func;
			job->jtype = FLY_TASK_PARALLEL_FOR;
		} else {
			fly_free(job);
			job = NULL;
		}
	}
	return job;
}

struct fly_job *fly_create_job_pfarr(int start, int end,
		fly_parallel_for_func func, void *data, size_t elsize)
{
	struct fly_job *job = NULL;
	if ((end - start) > 0) {
		job = fly_malloc(sizeof(struct fly_job));
		if (job) {
			if (fly_sem_init(&job->sem) == 0) {
				job->data = data;
				job->elsize = elsize;
				job->start = start;
				job->end = end;
				job->state = FLY_JOB_IDLE;
				fly_list_init(&job->batches);
				job->nbbatches = 0;
				job->batches_done = 0;
				job->func.pfor = func;
				job->jtype = FLY_TASK_PARALLEL_FOR_ARR;
			} else {
				fly_free(job);
				job = NULL;
			}
		}
	}
	return job;
}

struct fly_job *fly_create_job_task(struct fly_task *task)
{
	struct fly_job *job = fly_malloc(sizeof(struct fly_job));
	if (job) {
		if (fly_sem_init(&job->sem) == 0) {
			job->data = task;
			job->state = FLY_JOB_IDLE;
			fly_list_init(&job->batches);
			job->func.tfunc = task->func;
			job->nbbatches = 1;
			job->batches_done = 0;
			job->jtype = FLY_TASK_TASK;
		} else {
			fly_free(job);
			job = NULL;
		}
	}
	return job;
}

void fly_destroy_job(struct fly_job *job)
{
	if (job) {
		while (!fly_list_is_empty(&job->batches)) {
			struct fly_list *node = fly_list_tail(&job->batches);
			void *batch = node->el;
			job_remove_batch(job, batch);
			job_destroy_batch(batch);
		}
		fly_sem_uninit(&job->sem);
		fly_free(job);
	}
}

int fly_wait_job(struct fly_job *job)
{
	fly_assert(job, "fly_job_wait NULL job");
	/*
	 * TODO:
	 * Add variant to know if we wait from fly_thread so it could be tracked.
	 */
	if (fly_sem_notrack_wait(&job->sem) == 0) {
		fly_sched_job_collected(job);
		return FLYESUCCESS;
	}
	return FLYELLLIB;
}

int fly_make_batches(struct fly_job *job, int nbbatches)
{
	int funcscount = job->end - job->start;
	int batchsize = funcscount / nbbatches;
	int i;

	job->state = FLY_JOB_PREPARING;
	if (batchsize == 0)
		batchsize++;
	for (i = 0; i < nbbatches; i++) {
		int start = job->start + (i * batchsize);
		int end = start + batchsize;
		struct fly_job_batch *batch = job_make_batch(job, start, end);
		if (batch) {
			int err = job_add_batch(job, batch);
			if (!FLY_SUCCEEDED(err)) {
				job_destroy_batch(batch);
				break;
			}
		} else {
			break;
		}
	}
	if (i != nbbatches) {
		while (!fly_list_is_empty(&job->batches)) {
			struct fly_list *node = fly_list_tail(&job->batches);
			void *batch = node->el;
			job_remove_batch(job, batch);
			job_destroy_batch(batch);
		}
		job->state = FLY_JOB_IDLE;
		return FLYENORES;
	}
	if ((funcscount > nbbatches) && (funcscount % nbbatches) != 0) {
		struct fly_list *node = fly_list_tail(&job->batches);
		struct fly_job_batch *batch = node->el;
		batch->end += funcscount % nbbatches;
	}
	job->nbbatches = nbbatches;
	job->state = FLY_JOB_READY;
	return FLYESUCCESS;
}

void fly_destroy_batches(struct fly_job *job)
{
	if (job) {
		while (!fly_list_is_empty(&job->batches)) {
			struct fly_list *node = fly_list_tail(&job->batches);
			void *batch = node->el;
			job_remove_batch(job, batch);
			job_destroy_batch(batch);
		}
	}
}

struct fly_job_batch *fly_get_exec_batch(struct fly_job *job)
{
	struct fly_list *curr = fly_list_head(&job->batches);
	while (curr && curr->el) {
		struct fly_job_batch *batch = curr->el;
		if (fly_atomic_cas(&batch->execed, 0, 1))
			return batch;
		else
			curr = curr->next;
	}
	return NULL;
}

/******************************************************************************
 * Helper functions implementations.
 *****************************************************************************/
static struct fly_job_batch *job_make_batch(struct fly_job *job,
		int start, int end)
{
	struct fly_job_batch *batch = fly_malloc(sizeof(struct fly_job_batch));
	if (batch) {
		batch->start = start;
		batch->end = end;
		batch->job = job;
		batch->execed = 0;
	}
	return batch;
}

static int job_add_batch(struct fly_job *job, struct fly_job_batch *batch)
{
	return fly_list_append(&job->batches, batch);
}

static void job_remove_batch(struct fly_job *job, struct fly_job_batch *batch)
{
	fly_list_remove(&job->batches, batch);
}

static void job_destroy_batch(struct fly_job_batch *batch)
{
	fly_free(batch);
}
