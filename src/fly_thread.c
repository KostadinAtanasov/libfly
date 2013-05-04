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

#include "fly_thread.h"
#include <libfly/fly_error.h>
#include "fly_globals.h"

#include <sys/syscall.h>
#include <unistd.h>

/* Since we are using it only here... */
static pid_t	fly_threads_pid = 0;

static void *fly_thread_wrap_func(void *_thread)
{
	struct fly_thread *thread = (struct fly_thread*)_thread;

	thread->tid = syscall(SYS_gettid);
	thread->state = FLY_THREAD_RUNNING;

	return thread->func(thread->param);
}

int fly_thread_init(struct fly_thread *thread,
		fly_thread_func func, void *param)
{
	if (fly_threads_pid == 0)
		fly_threads_pid = getpid();

	thread->attr = fly_malloc(sizeof(pthread_attr_t));
	if (thread->attr) {
		if (pthread_attr_init(thread->attr) == 0) {
			thread->func = func;
			thread->param = param;
			thread->state = FLY_THREAD_IDLE;
			return FLYESUCCESS;
		} else {
			fly_free(thread->attr);
			return FLYEATTR;
		}
	}
	return FLYENORES;
}

int fly_thread_uninit(struct fly_thread *thread)
{
	pthread_attr_destroy(thread->attr);
	fly_free(thread->attr);
	return FLYESUCCESS;
}

int fly_thread_start(struct fly_thread *thread)
{
	int ret;
	ret = pthread_create(&thread->pthread, thread->attr,
			fly_thread_wrap_func, thread);
	return ret;
}

int fly_thread_wait(struct fly_thread *thread)
{
	pthread_join(thread->pthread, NULL);
	return FLYESUCCESS;
}
