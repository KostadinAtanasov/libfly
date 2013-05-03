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

#include <string.h>

#define FLY_WORKER_STATE_WAIT_NANOSEC 1000

void* fly_worker_func(void *worker)
{
	struct fly_worker *w = (struct fly_worker*)worker;
	while (w->tstate != FLY_WORKER_RUNNING)
		fly_thread_sleep(FLY_WORKER_STATE_WAIT_NANOSEC);

	while (w->tstate == FLY_WORKER_RUNNING) {
		fly_schedule(w);
	}

	w->tstate = FLY_WORKER_FINISHED;
	return w;
}

int fly_worker_init(struct fly_worker *worker)
{
	int err = FLYESUCCESS;

	memset(worker, 0, sizeof(struct fly_worker));

	worker->attr = fly_malloc(sizeof(pthread_attr_t));
	if (!worker->attr) {
		err = FLYENORES;
		goto OnError;
	}

	if (pthread_attr_init(worker->attr) != 0) {
		err = FLYEATTR;
		goto OnError;
	}

	err = sem_init(&worker->sem, 0, 0);
	if (err) {
		err = FLYENORES;
		goto OnError;
	}

	worker->tstate = FLY_WORKER_IDLE;

	return err;
	OnError:
		if (worker->attr) {
			pthread_attr_destroy(worker->attr);
			fly_free(worker->attr);
			worker->attr = NULL;
		}
		return err;
}

int fly_worker_uninit(struct fly_worker *worker)
{
	int err = FLYESUCCESS;

	if (worker->tstate == FLY_WORKER_RUNNING) {
		fly_worker_request_exit(worker);
		err = fly_worker_wait(worker);
	}

	if (FLY_SUCCEEDED(err)) {
		if (worker->attr) {
			pthread_attr_destroy(worker->attr);
			fly_free(worker->attr);
		}
		sem_destroy(&worker->sem);
	}
	return err;
}

int fly_worker_start(struct fly_worker *worker)
{
	int ret;
	ret = pthread_create(&worker->pthread, worker->attr,
		fly_worker_func, worker);
	if (ret == 0)
		worker->tstate = FLY_WORKER_RUNNING;
	return ret;
}

void fly_worker_request_exit(struct fly_worker *worker)
{
	worker->tstate = FLY_WORKER_EXITING;
	sem_post(&worker->sem);
}

int fly_worker_wait(struct fly_worker *worker)
{
	pthread_join(worker->pthread, NULL);
	return FLYESUCCESS;
}
