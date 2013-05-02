/******************************************************************************
 * fly_job.h
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

#ifndef FLY_JOB_H
#define FLY_JOB_H

#include <libfly/fly.h>
#include "fly_list.h"

#include <stdlib.h>
#include <semaphore.h>

enum fly_job_state {
	FLY_JOB_IDLE,
	FLY_JOB_PREPARING,
	FLY_JOB_READY,
	FLY_JOB_RUNNING,
	FLY_JOB_DONE
}; /* enum fly_job_state */

struct fly_job {
	struct fly_list		batches;
	int					nbbatches;
	volatile int		batches_done;

	union {
		fly_parallel_for_func	pfor;
		fly_task_func			tfunc;
	}					func;

	void				*data;
	size_t				elsize;
	int					start;
	int					end;
	int					jtype;
	enum fly_job_state	state;
	sem_t				sem;
}; /* struct fly_job */

struct fly_job_batch {
	struct fly_job	*job;
	int				start;
	int				end;
	int				execed;
}; /* struct fly_job_batch */

struct fly_job *fly_create_job_pfor(int count, fly_parallel_for_func func,
		void *ptr);
struct fly_job *fly_create_job_pfarr(int start, int end,
		fly_parallel_for_func func, void *data, size_t elsize);
struct fly_job *fly_create_job_task(struct fly_task *task);
void fly_destroy_job(struct fly_job *job);
int fly_wait_job(struct fly_job *job);
int fly_make_batches(struct fly_job *job, int nbbatches);
void fly_destroy_batches(struct fly_job *job);
struct fly_job_batch *fly_get_exec_batch(struct fly_job *job);

#endif /* FLY_JOB_H */
