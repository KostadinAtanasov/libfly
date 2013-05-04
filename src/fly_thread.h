/******************************************************************************
 * fly_thread.h
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

#ifndef FLY_THREAD_H
#define FLY_THREAD_H

#include <sys/types.h>
#include <pthread.h>

typedef void *(*fly_thread_func)(void*);

enum fly_thread_state {
	FLY_THREAD_IDLE				= 0x01,
	FLY_THREAD_RUNNING			= 0x02,
	FLY_THREAD_SLEEP			= 0x04,
	FLY_THREAD_CLIENT_RUNNING	= 0x08,
	FLY_THREAD_CLIENT_ISLEEP	= 0x10,	/* interruptable sleep(mutex, etc) */
	FLY_THREAD_CLIENT_USLEEP	= 0x20	/* uninterruptable sleep(IO) */
}; /* enum fly_thread_state */

struct fly_thread {
	pthread_t				pthread;
	pthread_attr_t			*attr;

	fly_thread_func			func;
	void					*param;

	enum fly_thread_state	state;

	/* thread id given by the OS */
	pid_t					tid;
}; /* struct fly_thread */

int fly_thread_init(struct fly_thread *thread,
		fly_thread_func func, void *param);
int fly_thread_uninit(struct fly_thread *thread);
int fly_thread_start(struct fly_thread *thread);

int fly_thread_wait(struct fly_thread *thread);

#endif /* FLY_THREAD_H */
